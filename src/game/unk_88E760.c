/*
 * Game overlay code at 0x88E760-0x895300 (to the end of the overlay's code), not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "stageobj.h"
#include "lobby_vobj.h"
/* prototypes of callees (round 13) */
s32 Obj_InCurrentArea(void *obj);
void Vec_Copy(f32 *out, f32 *in);
void DrawSlotB_Free(s32);

/* a part of a kind 0x1F object (0x20 bytes) */
typedef struct Obj1FPart {
    s16 slot;
    s8 t;                       /* 0x02: its frame, negative: not shown yet */
    s8 frame;                   /* 0x03: a texture frame (4 x 4) */
    s32 color;                  /* 0x04 */
    f32 sc[3];                  /* 0x08: its scale */
    DrawSlot *node;                /* 0x14 */
    u8 unk18[0x1E - 0x18];
    s16 angle;                  /* 0x1E: its z angle */
} Obj1FPart;

/* the work block of a kind 0x1F object */
typedef struct Obj1FWork {
    f32 m[16];                  /* 0x000: a matrix the type 0 and 4 parts are drawn in */
    f32 scale;                  /* 0x040 */
    Obj1FPart part[11];         /* 0x044 */
    u8 count;                   /* 0x1A4: parts in use */
    u8 unk1A5[0x1B7 - 0x1A5];
    u8 unk1B7;                  /* 0x1B7: type 5: nonzero draws without mode 2 */
    s32 angle[3];               /* 0x1B8: types 2 and 3 */
    s16 slot[3];                /* 0x1C4: type 3 */
    u8 unk1CA[2];
    DrawSlot *node[3];             /* 0x1CC */
} Obj1FWork;

/* a part of a kind 0x29 object (0x3C bytes, in its blocks) */
typedef struct Obj29Part {
    s16 slot;                   /* 0x00: its draw slot */
    u8 type;                    /* 0x02 */
    u8 unk03;
    f32 pos[3];                 /* 0x04 */
    u32 color;                  /* 0x10 */
    f32 sc[3];                  /* 0x14: times scale */
    f32 scale;                  /* 0x20: the object scale times the type factor */
    DrawSlot *node;                /* 0x24 */
    s16 unk28;                  /* 0x28 */
    u16 unk2A;                  /* 0x2A: angles (x, y, z) */
    u16 unk2C;                  /* 0x2C: 0 or 0x8000 (every other part) */
    u16 unk2E;                  /* 0x2E: a random angle */
    s16 unk30;                  /* 0x30 */
    s16 unk32;                  /* 0x32 */
    s16 unk34;                  /* 0x34: an angle speed */
    u8 unk36;                   /* 0x36 */
    u8 unk37;                   /* 0x37 */
    f32 unk38;                  /* 0x38 */
} Obj29Part;

/* a gust of a kind 0x27 object (0x14 bytes, ten in its blocks) */
typedef struct Obj27Part {
    s16 slot;                   /* 0x00: its draw slot */
    s8 t;                       /* 0x02: its frame, negative: a delay */
    s8 life;                    /* 0x03 */
    DrawSlot *node;                /* 0x04 */
    s16 unk08;                  /* 0x08: angles */
    s16 unk0A;                  /* 0x0A */
    s16 unk0C;                  /* 0x0C */
    s16 unk0E;                  /* 0x0E: an angle speed */
    f32 unk10;                  /* 0x10 */
} Obj27Part;

/* the work block of a kind 0x27 object */
typedef struct Obj27Work {
    Obj27Part part[10];         /* 0x00 */
    u8 unkC8[0xD0 - 0xC8];
    f32 m[16];                  /* 0xD0: the owner's bone 1 */
} Obj27Work;

/* the work block of a kind 0x2B object, the bow aim arc */
typedef struct Obj2BWork {
    s32 yaw;                    /* 0x00: of the owner's bone 0xE */
    s32 pitch;                  /* 0x04 */
    s32 unk08;                  /* 0x08 */
    s16 timer;                  /* 0x0C: counts to 24, then phase flips */
    u8 phase;                   /* 0x0E: 0 or 1 (the arc's colour) */
} Obj2BWork;

/* prototypes */
/* StageObj_Gun_Draw's key curves, models and lifetimes by part */
void Mtx_BillboardScaled(f32 *m, f32 *pos);
u8 Player_InCurrentArea(void *obj);
extern u8 *StageObj_Gun_ScaleKeyTbl[];
extern u8 *StageObj_Gun_ColorKeyTbl[];
extern u16 StageObj_Gun_ModelIds[];
extern s8 gStageObjGun_Lifetimes[];
extern u8 D_008D8AF8[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
extern f32 gCamMatrix[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
extern u8 StageObj_K27_ScaleKeys5[];
extern u8 StageObj_K27_ColorKeys5[];
extern u8 StageObj_K27_ScaleKeys9[];
extern u8 StageObj_K27_ColorKeys9[];
void BowgunShot_FlightParams(s16 idx, s32 *pos, f32 *a2, s16 *a3);
void Gfx_SetState63(s32);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_Concat3x3(f32 *m, f32 *n);
struct EmBone;
void Bone_UpdateWorld(struct EmBone *b, f32 *m);
void Bone_StoreMatrices(struct EmBone *b);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
f32 Math_Sin(f32 x);
s32 Player_IsLocal(void *obj);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 StageObj_K27_AllGustsDone(StageObj *o);
void StageObj_K27_FreeLastGusts(StageObj *o);
void Vec_RotateY(f32 *v, f32 angle);
extern Vec3f StageObj_K27_Vecs9[];
extern u16 StageObj_K25_Fade[];
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 f);
void StageObj_K25_Move(StageObj *o);
extern f32 StageObj_K25_Offset[];
extern u16 D_008D8BA2;
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Add(f32 *out, f32 *a, f32 *b);
f32 Math_Atan2(f32 y, f32 x);
void StageObj_K25_Draw(DrawSlot *node);
void StageObj_K27_Draw(DrawSlot *node);
void StageObj_K27_InitGust(Obj27Part *p, u8 i);
void Mtx_Copy(f32 *dst, f32 *src);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
extern s16 StageObj_K29_Frames1[];
extern f32 StageObj_K29_Scale3[];
extern f32 StageObj_K29_ScaleB2[];
extern u8 StageObj_K27_ModelSetIds[];
s16 EmSlot_FindKind(s16 k);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void StageObj_BowArc_Draw(DrawSlot *node);
s32 Bow_ArcVisible(void *p, u16 force);
EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
u16 Rand_Next(s32 stream);
void StageObj_K25_Setup(StageObj *o);
void StageObj_K25_Run(StageObj *o);
void StageObj_K25_End(StageObj *o);
void StageObj_K25_Free(StageObj *o);
void StageObj_K27_Setup(StageObj *o);
void StageObj_K27_Run(StageObj *o);
void StageObj_K27_End(StageObj *o);
void StageObj_K27_Free(StageObj *o);
void StageObj_K29_Setup(StageObj *o);
void StageObj_K29_Run(StageObj *o);
void StageObj_K29_End(StageObj *o);
void StageObj_K29_Free(StageObj *o);
void StageObj_K29_CreateOnPlayer(Player *p, u8 k, s32 c, f32 scale);
void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void StageObj_BowArc_Setup(StageObj *o);
void StageObj_BowArc_Run(StageObj *o);
void StageObj_BowArc_End(StageObj *o);
void StageObj_BowArc_Free(StageObj *o);
f32 Lure_RandUnit(void);
f32 Lure_RandSigned(void);
void DrawSlot_Free(s32);
extern s16 StageObj_K29_SpinDiv7[];
void StageObj_Gun_Update(StageObj *o);
extern s16 gStageObjK29_Counts[];
void Mtx_RotateXZY(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_Billboard(f32 *m, f32 *pos);
void Mtx_TurnX(f32 *m, f32 a);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
extern u16 *StageObj_K29_DrawTbl[];
extern f32 gCamYawMtx[];
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
u8 HuntingHorn_AttackNoteColour(Player *p);
void StageObj_K29_Draw(DrawSlot *node);
void StageObj_K29_PlaySound(StageObj *o);
void StageObj_K29_InitPart(StageObj *o, Obj29Part *w, EmBone *bone, s16 i);
void StageObj_K29_InitPartB(StageObj *o, Obj29Part *w, EmBone *bone, s16 i);
extern s16 StageObj_K29_SpinFrames7[];
extern s16 StageObj_K29_Delay0[];
extern s16 D_008D9008[];
extern u8 StageObj_K29_Types2[];
extern u8 StageObj_K29_Types3[];
extern u16 StageObj_K29_Delay3[];
extern s16 StageObj_K29_SpinFrames2[];
extern u8 StageObj_K29_Types5[];
extern u16 StageObj_K29_Delay5[];
extern s16 StageObj_K29_Delay7[];
extern u8 StageObj_K29_Types8[];
extern s16 StageObj_K29_Param28[];
extern u8 StageObj_K29_Unk03Pick[];
extern f32 StageObj_K29_Depth10[];
extern u8 StageObj_K29_Types10[];
extern u16 StageObj_K29_Delay10[];
extern s16 StageObj_K29_Frames1B[];
extern u8 StageObj_K29_Types11[];
extern s16 StageObj_K29_Delay11[];
extern u8 StageObj_K29_Types12[];
extern s16 StageObj_K29_Delay12[];
extern s16 StageObj_K29_Delay13[];
extern u8 StageObj_K29_Types14[];
extern s16 D_008D9E58[];
/* end of prototypes */

void StageObj_Gun_End(StageObj *o)
{
    Obj1FWork *w = (Obj1FWork *) o->buf;
    Obj1FPart *p;
    u8 i;

    o->state++;
    p = w->part;
    switch (o->type) {
    case 0:
        for (i = 0; i < w->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    case 1:
        for (i = 0; i < w->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    case 2:
        if (o->node != NULL) {
            DrawSlot_Free(o->slot);
        }
        break;
    case 3:
        for (i = 0; i < 3; i++) {
            if (w->node[i] != NULL) {
                DrawSlot_Free(w->slot[i]);
            }
        }
        break;
    case 4:
    case 5:
    case 6:
        for (i = 0; i < w->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    }
}

void StageObj_Gun_Free(StageObj *o)
{
    StageObj_Free(o);
}

/* Draws one part (node->unk1C) of a kind 0x1F object from the stage model set, by type: 0 the
 * muzzle flash (scale and colour from key curves), 1 the smoke and sparks (a 4 x 4 texture frame),
 * 2 and 3 the model 0x97 turned by angle[k], 4 the parts with their own lifetimes (gStageObjGun_Lifetimes), 5
 * while Player_InCurrentArea(owner) holds (part 2 spins, part 3 picks a random model), 6 the first 16 frames. */
void StageObj_Gun_Draw(DrawSlot *node)
{
    StageObj *o = node->owner;
    Obj1FWork *w = (Obj1FWork *) o->buf;
    Obj1FPart *p = &w->part[node->part];
    ModelMat *mats;
    u16 mode = 0;
    u8 id;
    Model *set = gStageModelSet;
    ModelMatSet *model;
    u8 *cc;
    f32 lim;
    u8 *sc;
    f32 m[16];
    f32 tm[16];
    f32 scale[3];
    f32 rot[3];
    s32 color;

    if (set != NULL && set->active && o->visible) {
    mats = set->mats;
    switch (o->type) {
    case 0:
        sc = StageObj_Gun_ScaleKeyTbl[node->part];
        cc = StageObj_Gun_ColorKeyTbl[node->part];
        model = &set->matSets[StageObj_Gun_ModelIds[node->part]];
        rot[0] = 0.0f;
        if (node->part == 1) {
            rot[1] = 0.5462889f;
        } else {
            rot[1] = 0.0f;
        }
        rot[2] = 0.0f;
        Key_LerpVec3((f32 *) sc, (f32 *) scale, o->timer);
        scale[0] *= w->scale;
        scale[1] *= w->scale;
        scale[2] *= w->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, m);
        if (node->part == 1) {
            Mtx_Concat3x3(m, w->m);
            Mtx_BillboardScaled(m, node->pos);
        } else {
            Mtx_Concat3x3(m, gCamMatrix);
        }
        Color_EvalKeys((s32 *) cc, o->timer, (u32 *) &color);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, m, color, 2, mats);
        Gs_SetState(0x6C, 1);
        break;
    case 1:
        switch (node->part) {
        case 0:
        case 1:
        case 2:
            id = 0x4A;
            lim = 30.0f;
            mode |= 4;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            break;
        case 3:
        case 4:
        case 5:
        case 9:
            lim = 25.0f;
            rot[2] = 0.0f;
            id = 0x4E;
            mode |= 2;
            break;
        case 6:
        case 7:
        case 8:
        case 10:
            id = 0x5E;
            lim = 3.0f;
            mode |= 2;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            break;
        }
        model = &set->matSets[id];
        if (p->t >= 0 && p->t <= lim) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            if ((u32) node->part >= 3 && (u32) node->part < 11) {
                Mtx_Translation(tm, 0.25f * (p->frame & 3), 0.25f * (p->frame / 4), 0.0f);
                Gs_SetState(0x19, (s32) tm);
            }
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, m, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 2:
    case 3:
        model = &set->matSets[0x97];
        if (o->type == 3) {
            rot[1] = 0.0f;
            rot[0] = 1.5707964f;
        } else {
            rot[0] = 0.0f;
            rot[1] = 1.5707964f;
        }
        rot[2] = (6.2831855f * w->angle[node->part]) / 65536.0f;
        scale[0] = 1.0f;
        scale[1] = 1.0f;
        scale[2] = 1.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, m);
        Effect_DrawModelAdd(model, (f32 *)((s32) m), (void *)((s32) mats));
        break;
    case 4:
        if (p->t >= 0 && p->t <= gStageObjGun_Lifetimes[node->part]) {
            switch (node->part) {
            case 0:
                mode |= 2;
                rot[2] = 0.0f;
                id = 0x11;
                rot[0] = -0.08726647f;
                rot[1] = 0.5462881f;
                break;
            case 1:
                rot[0] = 0.0f;
                id = 0x51;
                mode |= 2;
                rot[1] = 0.0f;
                rot[2] = 0.0f;
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                mode |= 4;
                id = 0x4A;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                break;
            case 6:
            case 7:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                mode |= 2;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                id = 0x5E;
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            }
            model = &set->matSets[id];
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, m);
            if (node->part == 0) {
                Mtx_Concat3x3(m, w->m);
                Mtx_BillboardScaled(m, node->pos);
            } else {
                Mtx_Concat3x3(m, gCamMatrix);
            }
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, m, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 5:
        if (Player_InCurrentArea(o->owner) & 0xFF) {
            switch (node->part) {
            case 0:
            case 1:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                id = 0x3E;
                break;
            case 2:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                id = 0x3E;
                p->angle += 0x1555;
                break;
            case 3:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                id = D_008D8AF8[Rand_Next(1) % 2];
                break;
            }
            model = &set->matSets[id];
            if (w->unk1B7 == 0) {
                mode |= 2;
            }
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, m, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 6:
        if (p->t >= 0 && p->t < 16) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            model = &set->matSets[0x4A];
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, m, p->color, 4, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    }
    }
}

StageObj *StageObj_Gun_Alloc(u8 k)
{
    return StageObj_Alloc(1);
}

void StageObj_Gun_CreateShell(void *owner, u8 lvl)
{
    StageObj *o;
    u8 i;

    if (Obj_InCurrentArea(owner)) {
        for (i = 0; i < 2; i++) {
            o = StageObj_Gun_Alloc(lvl);
            if (o == NULL) {
                break;
            }
            o->kind = 0x1F;
            o->update = StageObj_Gun_Update;
            o->owner = owner;
            o->type = i;
            switch (i) {
            case 0:
                switch (lvl) {
                case 0:
                    o->scale = 0.8f;
                    break;
                case 1:
                    o->scale = 1.2f;
                    break;
                case 2:
                    o->scale = 1.2f;
                    break;
                case 3:
                    o->scale = 2.0f;
                    break;
                }
                break;
            case 1:
                o->var2 = lvl;
                break;
            }
        }
    }
}

void StageObj_Gun_CreateReload(void *owner, u8 k)
{
    StageObj *o;

    if (Obj_InCurrentArea(owner)) {
        switch (k) {
        case 2:
            o = StageObj_Alloc(1);
            if (o != NULL) {
                o->kind = 0x1F;
                o->update = StageObj_Gun_Update;
                o->owner = owner;
                o->type = k;
            }
            break;
        case 3:
            o = StageObj_Alloc(1);
            if (o != NULL) {
                o->kind = 0x1F;
                o->update = StageObj_Gun_Update;
                o->owner = owner;
                o->type = k;
            }
            break;
        }
    }
}

void StageObj_Gun_CreateType4(void *owner)
{
    StageObj *o;

    if (Obj_InCurrentArea(owner)) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x1F;
            o->update = StageObj_Gun_Update;
            o->owner = owner;
            o->type = 4;
        }
    }
}

void StageObj_Gun_CreateType5(void *owner)
{
    StageObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x1F;
        o->update = StageObj_Gun_Update;
        o->owner = owner;
        o->type = 5;
    }
}

void StageObj_Gun_CreateType6(void *owner)
{
    StageObj *o;

    if (Obj_InCurrentArea(owner)) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x1F;
            o->update = StageObj_Gun_Update;
            o->owner = owner;
            o->type = 6;
        }
    }
}

void StageObj_K25_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K25_Setup(o);
        break;
    case 1:
        StageObj_K25_Run(o);
        break;
    case 2:
        StageObj_K25_End(o);
        break;
    case 3:
        StageObj_K25_Free(o);
        break;
    }
}

void StageObj_K25_Setup(StageObj *o)
{
    Player *p;
    u16 ang;
    u32 t;
    u16 da;
    u16 db;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 a[3];
    f32 b[3];

    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        p = &gPlayers[o->ownerIndex];
        Vec_Rotate(a, StageObj_K25_Offset, (f32 *) p->rotMtx);
        Vec_Add(a, a, p->pos);
        Vec_Rotate(b, o->pos, ((Em *) o->owner)->rotMtx[0]);
        Vec_Add(b, b, &((Em *) o->owner)->posX);
        o->node->pos[0] = a[0];
        o->node->pos[1] = a[1];
        o->node->pos[2] = a[2];
        dx = b[0] - a[0];
        dy = b[1] - a[1];
        dz = b[2] - a[2];
        o->angle2 = (s32) (0.5f + (65536.0f * Math_Atan2(dx, dz)) / 6.2831855f);
        o->angle2 += 0x4000;
        o->var2 = 0xD5;
        ang = ((Em *) o->owner)->rotY;
        ang += (u16) (s32) (0.5f + (65536.0f * (Rand_Next(1) % 50 - 25)) / 360.0f);
        da = ang - D_008D8BA2;
        db = o->angle2 + 0x1C72;
        da -= db;
        t = da + D_008D8BA2;
        if (t > 0xFFFF) {
            ang -= 0x8000;
        } else if (t > 0x8000) {
            ang -= 0x8000;
        }
        o->angle = ang;
        o->scale = b[1];
        o->pos[0] = dx / 21.5f;
        o->pos[2] = dz / 21.5f;
        o->pos[1] = 20.0f + 0.06666667f * dy;
        o->node->draw = StageObj_K25_Draw;
        return;
    }
    StageObj_Free(o);
}

void StageObj_K25_Run(StageObj *o)
{
    s16 t;
    u8 b;
    f32 v[3];
    f32 *py;

    if (++o->timer > 0x65) {
        o->state++;
        o->visible = 0;
        return;
    }
    switch (o->var0) {
    case 0:
        StageObj_K25_Move(o);
        t = o->timer;
        if (t <= 14) {
            o->angle2 += 0x1E5;
            o->var2 += 3;
        } else if (t <= 24) {
            o->angle2 += (u16) ((u16) ((u16) ((u16) o->angle - D_008D8BA2) - o->angle2) / (u16) (0x19 - (u16) t));
            b = o->var2;
            o->var2 = b + (u8) ((((u16) (-(StageObj_K25_Fade[0] + b) << 8) / (u16) (0x19 - (u16) o->timer)) & 0xFF00) >> 8);
        } else if (t <= 30) {
            o->angle2 += (u16) ((u16) ((u16) o->angle - o->angle2) / (u16) (0x1F - (u16) t));
            o->var2 += (u8) ((u8) -o->var2 / (u8) (0x1F - o->timer));
        }
        if (o->timer == 0x1C) {
            Vec_Copy(v, o->node->pos);
            v[1] += -40.0f;
            Effect0D_SpawnAt(v, 6, 1.0f);
        }
        if (o->timer >= 0x1F) {
            o->var0++;
            o->node->pos[1] = o->scale;
        } else if (*(py = &o->node->pos[1]) <= o->scale) {
            *py = o->scale;
            o->pos[0] *= 0.5f;
            o->pos[2] *= 0.5f;
            switch (o->var1) {
            case 0:
                o->pos[1] = 13.75f;
                Effect0D_SpawnAt(o->node->pos, 3, 1.0f);
                o->var1++;
                break;
            case 1:
                o->pos[1] = 8.75f;
                Effect0D_SpawnAt(o->node->pos, 3, 1.0f);
                o->var1++;
                break;
            }
        }
        break;
    case 1:
        break;
    }
    OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
}

void StageObj_K25_End(StageObj *o)
{
    o->state++;
    DrawSlot_Free(o->slot);
}

void StageObj_K25_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K25_Draw(DrawSlot *node)
{
    StageObj *o = node->owner;
    Model *mdl = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *mesh;
    f32 m[16];
    f32 a;

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        tex = mdl->mats;
        mesh = &mdl->matSets[58];
        Mtx_Scale(m, 1.5f, 1.5f, 1.5f);
        Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
        a = (6.2831855f * (f32) (o->var2 << 8)) / 65536.0f;
        Mtx_RotateXZY(m, a, (6.2831855f * o->angle2) / 65536.0f, a);
        Gs_SetState(0x67, -1);
        Effect_DrawModelAdd(mesh, (f32 *)((s32) m), tex);
    }
}

void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos)
{
    StageObj *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        o = StageObj_Alloc(0);
        if (o != NULL) {
            o->kind = 0x25;
            o->update = StageObj_K25_Update;
            o->owner = em;
            o->type = k;
            o->ownerIndex = p->index;
            Vec_Copy(o->pos, pos);
        }
    }
}

void StageObj_K25_Move(StageObj *o)
{
    o->pos[1] -= 2.5f;
    o->node->pos[0] += o->pos[0];
    o->node->pos[1] += o->pos[1];
    o->node->pos[2] += o->pos[2];
}

void StageObj_K27_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K27_Setup(o);
        break;
    case 1:
        StageObj_K27_Run(o);
        break;
    case 2:
        StageObj_K27_End(o);
        break;
    case 3:
        StageObj_K27_Free(o);
        break;
    }
}

void StageObj_K27_Setup(StageObj *o)
{
    Obj27Work *w = (Obj27Work *) o->buf;
    Obj27Part *p;
    s16 i = 0;

    o->state++;
    p = w->part;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->var0 = 0;
    o->node = NULL;
    o->slot = 10;
    o->angle = ((Em *) o->owner)->rotY;
    o->angle2 = 0;
    for (; i < 10; i++, p++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                StageObj_K27_InitGust(p, i);
                break;
            case 6:
            case 7:
            case 8:
            case 9:
                p->t = 0;
                p->life = 10.0f + 5.0f * (u32) Lure_RandSigned();
                p->unk10 = 45.0f;
                p->unk08 = 0;
                p->unk0A = 0;
                p->unk0C = 0.5f + (65536.0f * (360.0f * Lure_RandUnit())) / 360.0f;
                p->unk0E = 0.5f + (65536.0f * ((30.0f * Lure_RandSigned()) / 30.0f)) / 360.0f;
                break;
            }
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K27_Draw;
            Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 1)));
            Mtx_GetTrans(p->node->pos, w->m);
        } else {
            p->node = NULL;
        }
    }
    StageObj_K27_Run(o);
}

void StageObj_K27_InitGust(Obj27Part *p, u8 i)
{
    p->unk10 = (f32) ((200.0f + (100.0f * Lure_RandUnit())) / 30.0f);
    p->t = i * -5;
    p->unk08 = (s16) (s32) (0.5f + ((65536.0f * (70.0f + (50.0f * Lure_RandUnit()))) / 360.0f));
    p->unk0A = (s16) (s32) (0.5f + ((65536.0f * (360.0f * Lure_RandUnit())) / 360.0f));
    p->unk0C = 0;
    p->unk0E = (u16) (s32) (0.5f + ((65536.0f * ((10.0f * Lure_RandSigned()) / 30.0f)) / 360.0f));
}

void StageObj_K27_Run(StageObj *o)
{
    Obj27Work *w = (Obj27Work *) o->buf;
    Obj27Part *p = w->part;
    Em *em = o->owner;
    u8 i;
    Vec3f v;

    if ((em->motionId[0] != 0x424 || EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && ((Em *) o->owner)->motionId[0] != 0x423) {
        o->var0 = 1;
    }
    if (o->timer >= 30) {
        if (o->var0 == 1 && StageObj_K27_AllGustsDone(o) == 1) {
            o->state++;
        } else if (o->angle2 == 0) {
            o->angle2 = 1;
            StageObj_K27_FreeLastGusts(o);
        }
    } else {
        o->timer++;
    }
    for (i = 0; i < o->slot; i++, p++) {
        if (p->node == NULL) {
            continue;
        }
        if (p->t++ < 0) {
            continue;
        }
        if (p->t >= 30) {
            if (o->var0 != 1) {
                StageObj_K27_InitGust(p, i);
                Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 1)));
                Mtx_GetTrans(p->node->pos, w->m);
                p->t = -5;
            }
            continue;
        }
        switch (i) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            p->unk0A += p->unk0E;
            p->node->pos[1] += p->unk10;
            OT_AddByDepth(gDrawOt32, p->node, 0x40, 0);
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            p->unk0C += p->unk0E;
            v = StageObj_K27_Vecs9[i - 6];
            v.x *= p->unk10;
            v.z *= p->unk10;
            if (p->t >= p->life) {
                v.y = 7.5f;
            } else {
                v.y = 0.0f;
            }
            Vec_RotateY(&v.x, (6.2831855f * o->angle) / 65536.0f);
            p->unk10 -= 1.5f;
            p->node->pos[0] += v.x;
            p->node->pos[1] += v.y;
            p->node->pos[2] += v.z;
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            break;
        }
    }
}

s32 StageObj_K27_AllGustsDone(StageObj *o)
{
    s16 n = o->slot;
    u8 *p = o->buf;
    u8 i;

    for (i = 0; i < n; i++) {
        if (*(s8 *) (p + 2) < 30) {
            return 0;
        }
        p += 0x14;
    }
    return 1;
}

void StageObj_K27_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 i;

    for (i = 0; i < 10; i++) {
        if (*(s32 *) (p + 4) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x14;
    }
    o->state++;
}

void StageObj_K27_FreeLastGusts(StageObj *o)
{
    u8 *p = o->buf + 0x78;
    s16 i;

    for (i = 0; i < 4; i++) {
        if (*(s32 *) (p + 4) != 0) {
            *(s32 *) (p + 4) = 0;
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x14;
    }
    o->slot = 6;
}

void StageObj_K27_Free(StageObj *o)
{
    StageObj_Free(o);
}

s16 StageObj_K27_FindModelSet(void)
{
    u8 *p = StageObj_K27_ModelSetIds;
    u8 v = *p;
    s16 r;

    while (v != 0xFF) {
        r = EmSlot_FindKind(v);
        if (r != -1) {
            return r;
        }
        p++;
        v = *p;
    }
    return -1;
}

void StageObj_K27_Draw(DrawSlot *node)
{
    s32 color;
    StageObj *o = node->owner;
    f32 scale[3];
    Obj27Part *p = (Obj27Part *) o->buf + node->part;
    ModelMatSet *model;
    f32 rot[3];
    f32 m[16];
    f32 tm[16];
    u8 *sc;
    u16 mode = 0;
    ModelMat *mats;
    u8 *cc;
    f32 v;
    Model *set;
    s16 k;

    k = StageObj_K27_FindModelSet();
    if (k != -1 && (set = gScene.partModels[k]) != NULL && set->active && o->visible) {
        switch (node->part) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            mode |= 2;
            model = &set->matSets[7];
            sc = StageObj_K27_ScaleKeys5;
            cc = StageObj_K27_ColorKeys5;
            v = p->t / 30;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            model = &set->matSets[1];
            mode |= 4;
            sc = StageObj_K27_ScaleKeys9;
            cc = StageObj_K27_ColorKeys9;
            v = 0.0f;
            break;
        }
        mats = set->mats;
        Key_LerpVec3((f32 *) sc, (f32 *) scale, p->t);
        rot[0] = (6.2831855f * p->unk08) / 65536.0f;
        rot[1] = (6.2831855f * p->unk0A) / 65536.0f;
        rot[2] = (6.2831855f * p->unk0C) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, m);
        if ((u32) node->part >= 3) {
            Mtx_Concat3x3(m, gCamMatrix);
        }
        Color_EvalKeys((s32 *) cc, p->t, (u32 *) &color);
        Mtx_Translation(tm, 0.0f, v, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, m, color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K27_Create(Em *em)
{
    StageObj *o;

    if (Player_InCurrentArea(em)) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x27;
            o->update = StageObj_K27_Update;
            o->owner = em;
            o->type = 0;
        }
    }
}

void StageObj_K29_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K29_Setup(o);
        break;
    case 1:
        StageObj_K29_Run(o);
        break;
    case 2:
        StageObj_K29_End(o);
        break;
    case 3:
        StageObj_K29_Free(o);
        break;
    }
}

void StageObj_K29_Setup(StageObj *o)
{
    Obj29Part *p = (Obj29Part *) o->buf;
    Player *owner = o->owner;
    s16 n;
    s16 i;
    s16 k;
    u16 bits;
    f32 m[16];
    f32 v[3];

    o->state++;
    o->var0 = 0;
    if (o->owner != NULL) {
        o->var2 = 0;
    } else {
        o->var2 = 0xFF;
    }
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    n = gStageObjK29_Counts[o->type];
    switch (o->type) {
    case 0:
    case 1:
    case 4:
        Em_GetBonePos(owner, 10, o->pos);
        break;
    case 2:
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(owner, 0x12)));
        break;
    case 3:
        bits = Rand_Next(1);
        break;
    case 6:
        Em_GetBonePos(owner, 10, o->pos);
        o->pos[1] = owner->floorY + 30.0f * o->scale;
        bits = Rand_Next(1);
        switch (o->var1) {
        case 1:
            o->var1 = 0;
            break;
        case 2:
            o->var1 = 1;
            o->scale *= 1.8f;
            break;
        default:
            StageObj_K29_Free(o);
            return;
        }
        break;
    case 8:
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(owner, 10)));
        break;
    case 9:
        Em_GetBonePos(owner, 10, o->pos);
        bits = Rand_Next(1);
        break;
    case 10:
        Vec_Copy(o->pos, ((Player *) o->owner)->pos);
        v[0] = 25.0f;
        v[1] = 0.0f;
        v[2] = 120.0f;
        Vec_RotateY(v, (6.2831855f * ((Player *) o->owner)->rot[1]) / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        o->angle2 = ((Player *) o->owner)->rot[1];
        break;
    case 11:
        o->var1 = HuntingHorn_AttackNoteColour(o->owner);
        Mtx_GetTrans(o->pos, owner->weaponModel->partBones[0][3].world);
        break;
    case 14:
        bits = Rand_Next(1);
        k = 0;
        break;
    case 15:
        bits = Rand_Next(1);
        k = 0;
        break;
    }
    StageObj_K29_PlaySound(o);
    for (i = 0; i < n; i++, p++) {
        p->unk28 = 0;
        p->type = i;
        p->unk03 = 0;
        p->scale = o->scale;
        p->unk36 = 1;
        p->unk37 = 1;
        p->unk2A = 0;
        p->unk2C = 0;
        p->unk2E = 0;
        p->unk30 = 0;
        p->unk32 = 0;
        p->unk34 = 0;
        p->unk38 = 0.0f;
        switch (o->type) {
        case 0:
            Vec_Copy(p->pos, o->pos);
            p->unk28 = -StageObj_K29_Delay0[i];
            switch (i) {
            case 0:
            case 1:
            case 3:
                break;
            case 2:
                p->unk2E = Rand_Next(1);
                break;
            case 4:
                p->unk37 = 0;
                p->pos[1] = owner->floorY;
                break;
            default:
            case 5:
            case 6:
            case 7:
                p->type = 5;
                p->unk2A = Rand_Next(1);
                p->unk2C = Rand_Next(1);
                p->unk2E = Rand_Next(1);
                bits = Rand_Next(1);
                p->unk34 = ((bits & 0xFFF) + 0x1555) / (f32) StageObj_K29_SpinFrames7[p->type];
                if (bits & 0x1000) {
                    p->unk34 = -p->unk34;
                }
                break;
            }
            break;
        case 4:
            Vec_Copy(p->pos, o->pos);
            p->unk28 = -StageObj_K29_Delay0[i];
            switch (i) {
            case 0:
            case 1:
                break;
            case 2:
                p->unk2E = Rand_Next(1);
                break;
            case 3:
                p->type = 4;
                p->unk37 = 0;
                p->pos[1] = owner->floorY;
                break;
            }
            break;
        case 1:
            p->unk28 = -D_008D9008[i];
            switch (i) {
            case 0:
            case 1:
                p->type = 0;
                break;
            case 2:
                p->type = 1;
                break;
            case 3:
            case 4:
                p->type = 2;
                p->unk38 = 0.0f;
                break;
            case 5:
                p->type = 3;
                break;
            case 6:
                p->type = 4;
                p->unk38 = 0.0f;
                break;
            }
            break;
        case 2:
            p->type = StageObj_K29_Types2[i];
            StageObj_K29_InitPart(o, p, (EmBone *) m, i);
            break;
        case 3:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types3[i];
            p->unk28 = -StageObj_K29_Delay3[i];
            switch (p->type) {
            case 0:
                p->unk2E = Rand_Next(1);
                break;
            case 1:
                p->unk2A = o->angle2;
                p->unk2C = o->angle;
                break;
            case 2:
                p->unk2A = (Rand_Next(1) & 0xFFF) - 0x800;
                p->unk2C = o->angle + (Rand_Next(1) & 0xFFF) - 0x800;
                p->unk2E = Rand_Next(1);
                p->unk34 = ((u16) (s32) (0.5f + (65536.0f * (40.0f * Lure_RandUnit())) / 360.0f) + 0x1C72) / (f32) StageObj_K29_SpinFrames2[p->type];
                if (bits & (1 << i)) {
                    p->unk34 = -p->unk34;
                }
                break;
            }
            break;
        case 5:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types5[i];
            p->unk28 = -StageObj_K29_Delay5[i];
            switch (p->type) {
            case 0:
                p->pos[1] += 20.0f;
                p->unk2A = 0x4000;
                break;
            default:
            case 1:
                p->unk37 = 0;
                p->pos[1] += 10.0f;
                break;
            }
            break;
        case 6:
        case 9:
            Vec_Copy(p->pos, o->pos);
            p->pos[0] += o->scale * (50.0f * Lure_RandSigned());
            p->pos[2] += o->scale * (50.0f * Lure_RandSigned());
            p->type = bits & 1;
            p->unk2E = bits;
            p->unk34 = 0x5B;
            if ((bits >> 1) & 1) {
                p->unk34 = -p->unk34;
            }
            break;
        case 7:
            p->type = 0;
            p->unk28 = -StageObj_K29_Delay7[i];
            p->scale = 0.8f + 0.7f * Lure_RandUnit();
            p->unk2E = Rand_Next(1);
            p->unk34 = 0.5f + (65536.0f * ((360.0f * Lure_RandSigned()) / StageObj_K29_SpinDiv7[p->type])) / 360.0f;
            break;
        case 8:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types8[i];
            StageObj_K29_InitPartB(o, p, (EmBone *) m, i);
            switch (p->type) {
            case 2:
            case 3:
                p->unk28 = StageObj_K29_Param28[p->type];
                break;
            }
            break;
        case 10:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types10[i];
            p->unk28 = -StageObj_K29_Delay10[i];
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = StageObj_K29_Depth10[i % 3];
            Vec_RotateY(v, (6.2831855f * o->angle2) / 65536.0f);
            p->pos[0] += v[0];
            p->pos[1] += v[1];
            p->pos[2] += v[2];
            switch (p->type) {
            case 0:
                p->unk2E = 0x4000;
                p->unk37 = 1;
                break;
            case 1:
                p->unk03 = StageObj_K29_Unk03Pick[Rand_Next(1) % 2];
                p->unk38 = (10.0f + 10.0f * Lure_RandUnit()) / StageObj_K29_Frames1B[p->type];
                p->unk2E = Rand_Next(1);
                p->unk34 = 0.5f + (65536.0f * ((10.0f * Lure_RandSigned()) / (u8) StageObj_K29_Frames1B[p->type])) / 360.0f;
                p->unk37 = 1;
                break;
            }
            break;
        case 11:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types11[i];
            p->unk28 = -StageObj_K29_Delay11[i];
            switch (p->type) {
            case 0:
                p->unk2E = Rand_Next(1);
                break;
            case 1:
                break;
            }
            break;
        case 12:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types12[i];
            p->unk28 = -StageObj_K29_Delay12[i];
            if (o->var1 == 0) {
                p->unk28 -= 5;
            }
            p->unk2E = Rand_Next(1);
            switch (p->type) {
            case 0:
                break;
            case 1:
                p->unk38 = 2.0f;
                p->pos[0] += 50.0f * Lure_RandSigned();
                p->pos[1] += 50.0f * Lure_RandSigned();
                p->pos[2] += 50.0f * Lure_RandSigned();
                break;
            case 2:
                p->type += Rand_Next(1) & 1;
                p->unk38 = 0.2f;
                p->pos[0] += 30.0f * Lure_RandSigned();
                p->pos[1] += 30.0f * Lure_RandSigned();
                p->pos[2] += 30.0f * Lure_RandSigned();
                break;
            }
            break;
        case 13:
            Vec_Copy(p->pos, o->pos);
            p->unk28 = -StageObj_K29_Delay13[i];
            switch (p->type) {
            case 1:
                p->unk03 = Rand_Next(1) & 3;
                break;
            }
            break;
        case 14:
            Vec_Copy(p->pos, o->pos);
            p->type = StageObj_K29_Types14[i];
            p->unk28 = -D_008D9E58[i];
            switch (p->type) {
            case 0:
                if (i == 1) {
                    p->unk2A = 0x4000;
                    p->unk03 = 1;
                }
                break;
            case 1:
                break;
            case 2:
                p->unk37 = 0;
                break;
            case 3:
                p->type += (bits >> k++) & 1;
                p->unk2E = Rand_Next(1);
                p->pos[0] += o->scale * (50.0f * Lure_RandSigned());
                p->pos[1] += o->scale * (80.0f + 50.0f * Lure_RandSigned());
                p->pos[2] += o->scale * (50.0f * Lure_RandSigned());
                p->unk38 = o->scale * ((100.0f + 50.0f * Lure_RandUnit()) / 30.0f);
                break;
            }
            break;
        case 15:
            Vec_Copy(p->pos, o->pos);
            p->unk28 = i * -4;
            p->pos[0] += o->scale * (15.0f * Lure_RandSigned());
            p->pos[2] += o->scale * (15.0f * Lure_RandSigned());
            p->type = (bits >> k++) & 1;
            p->unk2E = Rand_Next(1);
            p->unk34 = 0x111;
            if ((bits >> k++) & 1) {
                p->unk34 = -p->unk34;
            }
            break;
        case 16:
            p->type = 0;
            p->scale *= 1.5f;
            Vec_Copy(p->pos, o->pos);
            p->unk2E = Rand_Next(1);
            break;
        }
        if (o->keep == 0) {
            p->slot = DrawSlot_Alloc();
        } else {
            p->slot = DrawSlotB_Alloc();
        }
        if (p->slot != -1) {
            if (o->keep == 0) {
                p->node = DrawSlot_Get(p->slot);
            } else {
                p->node = DrawSlotB_Get(p->slot);
            }
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K29_Draw;
        } else {
            p->node = NULL;
        }
    }
}

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 StageObj_K29_Life[];
extern s16 *StageObj_K29_Durations[];
extern s16 StageObj_K29_CurveIdx[];
extern s16 StageObj_K29_ColorIdx[];
extern void *StageObj_K29_Curves[];
extern u8 *StageObj_K29_ColorKeys[];
extern u8 StageObj_K29_Kinds3[];
extern u8 StageObj_K29_Alpha2[];
extern f32 StageObj_K29_Vec2[];
extern s16 D_008D8FF8[];
s32 StageObj_K29_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_AttackNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void StageObj_K29_RandomizeSpin(Obj29Part *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

void StageObj_K29_Run(StageObj *o)
{
    Obj29Part *p = (Obj29Part *) o->buf;
    Player *owner = o->owner;
    s16 life;
    s16 n;
    s16 i;
    s16 lim;
    s32 t;
    s16 kt;
    s16 ki;
    s32 ci;
    s16 d;
    void *curve;
    s16 *durs;
    u8 prev;
    u16 bits;
    f32 m[16];
    f32 r[3];
    f32 v[3];
    f32 a;
    u16 yaw;
    u16 pitch;

    life = StageObj_K29_Life[o->type];
    o->timer++;
    if (life != 1000 && o->timer > life) {
        o->state++;
        o->visible = 0;
        return;
    }
    n = gStageObjK29_Counts[o->type];
    durs = StageObj_K29_Durations[o->type];
    if (o->var2 != 0xFF && StageObj_K29_OwnerGone(owner) == 1) {
        o->var2 = 0xFF;
    }
    switch (o->type) {
    case 2:
        if (o->var2 != 0xFF) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(owner, 0x12)));
            Vec_DirToAngles(m, (s16 *) &yaw, (s16 *) &pitch);
            prev = o->var2;
            o->var2 = Bow_ChargeLevel(owner);
            if (((Player *) o->owner)->motionId[0] != 0x57D && ((Player *) o->owner)->motionId[0] != 0x3F1 && ((Player *) o->owner)->motionId[0] != 0x3F4 && ((Player *) o->owner)->motionId[0] != 0x3F5 && ((Player *) o->owner)->motionId[0] != 0x58A && ((Player *) o->owner)->motionId[0] != 0x58B && ((Player *) o->owner)->motionId[0] != 0x58C) {
                o->var2 = 0xFF;
            }
        }
        if (o->var2 == 0xFF) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (Bow_ArcVisible(owner, 1) == 1) {
            o->visible = 1;
        } else {
            o->visible = 0;
        }
        if (prev != o->var2) {
            switch (o->var2) {
            case 1:
                Player_PlaySe(o->owner, 0xC, 0, &m[12], 1, 0);
                break;
            case 2:
                Player_PlaySe(o->owner, 0xD, 0, &m[12], 1, 0);
                break;
            case 3:
                Player_PlaySe(o->owner, 0xE, 0, &m[12], 1, 0);
                Player_PlaySe(o->owner, 0x12, 0, &m[12], 1, 0);
                break;
            }
        }
        switch (o->var2) {
        case 3:
            if (o->var1 < 0xFF) {
                o->var1++;
            }
            break;
        }
        break;
    case 7:
        if (o->var2 == 0xFF) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (!(Obj_InCurrentArea(owner) & 0xFF) || !(HuntingHorn_AttackNote(owner) & 0xFF)) {
            return;
        }
        Mtx_GetTrans(o->pos, owner->weaponModel->partBones[0][3].world);
        break;
    case 8:
        if (o->var2 != 0xFF) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(owner, 10)));
            prev = o->var2;
            o->var2 = GreatSword_ChargeLevel(owner);
            if (((Player *) o->owner)->motionId[0] != 0x579 && ((Player *) o->owner)->motionId[0] != 0x58C && ((Player *) o->owner)->motionId[0] != 0x58A && ((Player *) o->owner)->motionId[0] != 0x590 && ((Player *) o->owner)->motionId[0] != 0x592 && ((Player *) o->owner)->motionId[0] != 0x595 && ((Player *) o->owner)->motionId[0] != 0x596) {
                o->var2 = 0xFF;
            }
            Mtx_GetTrans(o->pos, m);
        }
        if (o->var2 == 0xFF) {
            o->state++;
            o->visible = 0;
            Player_PlaySe(o->owner, 0x1E, 0, ((Player *) o->owner)->pos, 1, 0);
            Player_PlaySe(o->owner, 0x1F, 0, ((Player *) o->owner)->pos, 1, 0);
            return;
        }
        if (o->timer == 0x2D) {
            Player_PlaySe(o->owner, 0x1D, 0, &m[12], 1, 0);
        }
        break;
    case 12:
        if (o->var1 == 0 && o->timer == 5) {
            Sound_PlaySeAtPosChecked(1, 0xB7, 0, o->pos, 1, 0);
        }
        break;
    case 15:
        if (o->timer > o->var1) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (o->timer % 14 == 1) {
            Sound_PlaySeAtPosChecked(7, 10, 0, o->pos, 0x11, 0);
        }
        break;
    }
    for (i = 0; i < n; i++, p++) {
        lim = durs[p->type];
        switch (o->type) {
        case 2:
            if (prev != o->var2) {
                switch (o->var2) {
                case 1:
                case 2:
                    switch (p->type) {
                    case 3:
                    case 4:
                        p->unk28 = 0;
                        StageObj_K29_InitPart(o, p, (EmBone *) m, i);
                        break;
                    }
                    break;
                case 3:
                    switch (StageObj_K29_Kinds3[i]) {
                    case 5:
                    case 7:
                        break;
                    default:
                        p->type = StageObj_K29_Kinds3[i];
                        StageObj_K29_InitPart(o, p, (EmBone *) m, i);
                        break;
                    }
                    break;
                }
            }
            if (o->var2 == 3 && o->var1 == 5) {
                switch (StageObj_K29_Kinds3[i]) {
                case 5:
                case 7:
                    p->type = StageObj_K29_Kinds3[i];
                    StageObj_K29_InitPart(o, p, (EmBone *) m, i);
                    break;
                }
            }
            break;
        case 7:
            if (p->type == 0xFF) {
                continue;
            }
            break;
        case 8:
            if (prev != o->var2) {
                switch (o->var2) {
                case 1:
                case 2:
                    switch (p->type) {
                    case 2:
                    case 3:
                        p->unk28 = 0;
                        StageObj_K29_InitPartB(o, p, (EmBone *) m, i);
                        break;
                    }
                    break;
                }
            }
            break;
        case 10:
            switch (p->type) {
            case 0:
            if (o->timer > lim + StageObj_K29_Delay10[i]) {
                Vec_Copy(p->pos, o->pos);
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = StageObj_K29_Depth10[i % 3];
                Vec_RotateY(v, (6.2831855f * o->angle2) / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += v[1];
                p->pos[2] += v[2];
                p->type = StageObj_K29_Types10[i + 6];
                p->unk28 = -StageObj_K29_Delay10[i + 6];
                p->unk37 = 0;
                p->unk38 = 15.0f;
                p->unk2E = Rand_Next(1);
            }
            break;
            }
            break;
        }
        if (++p->unk28 > lim) {
            switch (o->type) {
            case 1:
                switch (p->type) {
                case 0:
                    p->type = 2;
                    switch (i) {
                    case 0:
                        p->unk28 = -9;
                        break;
                    case 1:
                        p->unk28 = -13;
                        break;
                    }
                    break;
                case 2:
                    if (o->timer < 0x29) {
                        p->unk28 = 1;
                    }
                    break;
                case 4:
                    if (o->timer < 0x2E) {
                        p->unk28 = 1;
                    }
                    break;
                }
                break;
            case 2:
                switch (p->type) {
                case 0:
                    p->unk28 = 0x1F;
                    break;
                case 1:
                    p->unk28 = 1;
                    p->unk2E = Rand_Next(1);
                    break;
                case 2:
                case 5:
                case 6:
                    p->unk28 = 1;
                    break;
                }
                break;
            case 7:
                StageObj_K29_RandomizeSpin(p);
                break;
            case 8:
                switch (p->type) {
                case 0:
                case 1:
                    p->unk28 = StageObj_K29_Param28[p->type];
                    break;
                default:
                    p->unk28 = StageObj_K29_Param28[p->type] + 1;
                    break;
                }
                break;
            case 15:
                if (o->timer < o->var1 - 8) {
                    Vec_Copy(p->pos, o->pos);
                    p->pos[0] += o->scale * (15.0f * Lure_RandSigned());
                    p->pos[2] += o->scale * (15.0f * Lure_RandSigned());
                    p->unk28 = -2;
                    bits = Rand_Next(1);
                    p->type = bits & 1;
                    p->unk2E = bits;
                    if ((bits >> 1) & 1) {
                        p->unk34 = -p->unk34;
                    }
                } else {
                    p->unk28 = lim;
                }
                break;
            }
        }
        if (p->unk28 > 0 && p->unk28 <= lim) {
            switch (o->type) {
            case 1:
                if (p->unk28 == 1) {
                    p->unk2E = Rand_Next(1);
                    Em_GetBonePos(o->owner, 10, p->pos);
                    switch (p->type) {
                    case 2:
                        bits = Rand_Next(1);
                        p->unk34 = 2731.0f / D_008D8FF8[p->type];
                        if (bits & 1) {
                            p->unk34 = -p->unk34;
                        }
                        Vec_Copy(r, StageObj_K29_Vec2);
                        r[0] *= Lure_RandSigned();
                        r[1] *= Lure_RandSigned();
                        r[2] *= Lure_RandSigned();
                        Vec_Add(p->pos, p->pos, r);
                        break;
                    case 3:
                        Vec_Copy(r, StageObj_K29_Vec2);
                        r[0] *= Lure_RandSigned();
                        r[1] *= Lure_RandSigned();
                        r[2] *= Lure_RandSigned();
                        Vec_Add(p->pos, p->pos, r);
                        break;
                    }
                }
                break;
            case 8:
                if (p->unk28 == 1) {
                    switch (p->type) {
                    case 2:
                        Player_PlaySe(o->owner, 0xD, 0, &m[12], 1, 0);
                        break;
                    }
                }
                break;
            }
            ki = StageObj_K29_CurveIdx[o->type];
            ci = StageObj_K29_ColorIdx[o->type];
            t = p->unk28;
            kt = t;
            switch (o->type) {
            case 6:
            case 9:
                ci = (s16) (ci + o->var1);
                break;
            case 8:
                switch (p->type) {
                case 0:
                    kt = (t & 3) + 1;
                    break;
                }
                d = p->type;
                ki += d;
                ci = (s16) (ci + d);
                break;
            case 11:
                switch (p->type) {
                case 0:
                    ci = (s16) (o->var1 - 1 + ci);
                    break;
                case 1:
                    ci = (s16) (o->var1 - 1 + (ci + 6));
                    break;
                }
                break;
            case 16:
                d = p->type + o->var1;
                ki += d;
                ci = (s16) (ci + d);
                break;
            default:
                d = p->type;
                ki += d;
                ci = (s16) (ci + d);
                break;
            }
            if (p->unk37) {
                curve = StageObj_K29_Curves[ki];
                ki++;
                Key_LerpVec2(curve, p->sc, kt);
            } else {
                curve = StageObj_K29_Curves[ki];
                ki++;
                Key_LerpVec3(curve, p->sc, kt);
            }
            if (p->unk36 == 0) {
                Key_LerpF32(StageObj_K29_Curves[ki], &a, (s16) t);
                p->color = ((u8) (u32) (255.0f * a) << 24) | 0xFFFFFF;
            } else {
                Color_EvalKeys((s32 *) (StageObj_K29_ColorKeys[(s16) ci]), (s16) t, (u32 *) &p->color);
            }
            switch (o->type) {
            case 0:
            case 4:
                switch (p->type) {
                case 0:
                case 1:
                case 2:
                case 3:
                    Em_GetBonePos(o->owner, 10, p->pos);
                    break;
                }
                break;
            case 1:
                switch (p->type) {
                case 2:
                    p->unk38 += -0.3f;
                    p->pos[1] += p->unk38;
                    break;
                case 4:
                    p->unk38 += -0.5f;
                    p->pos[1] += p->unk38;
                    break;
                default:
                    Em_GetBonePos(o->owner, 10, p->pos);
                    break;
                }
                break;
            case 2:
                switch (p->type) {
                default:
                    v[0] = 0.0f;
                    break;
                case 1:
                    v[0] = -p->unk38 * p->unk28;
                    break;
                case 2:
                    v[0] = -150.0f;
                    p->color = (p->color & 0xFF) | ((((p->color >> 8) & 0xFF) << 8) | ((StageObj_K29_Alpha2[o->var2] << 24) | (((p->color >> 16) & 0xFF) << 16)));
                    p->unk2A = yaw;
                    p->unk2C = pitch;
                    break;
                case 3:
                    v[0] = -110.0f;
                    break;
                case 4:
                    v[0] = -150.0f;
                    p->unk2A = yaw;
                    p->unk2C = pitch;
                    break;
                case 5:
                    v[0] = -150.0f;
                    break;
                case 6:
                    v[0] = -175.0f;
                    p->unk2A = yaw;
                    p->unk2C = pitch;
                    break;
                case 7:
                    v[0] = -110.0f;
                    break;
                case 8:
                    v[0] = -150.0f;
                    p->unk2A = yaw;
                    p->unk2C = pitch;
                    break;
                case 9:
                    v[0] = -150.0f;
                    break;
                }
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateSelf((f32 *) v, (f32 *) m);
                Mtx_GetTrans(p->pos, m);
                p->pos[0] += v[0];
                p->pos[1] += v[1];
                p->pos[2] += v[2];
                break;
            case 3:
                switch (p->type) {
                case 2:
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 150.0f / lim;
                    Vec_RotateX(v, (6.2831855f * o->angle2) / 65536.0f);
                    Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += v[1];
                    p->pos[2] += v[2];
                    break;
                }
                break;
            case 6:
            case 9:
                p->pos[1] += 3.0f * o->scale;
                break;
            case 7:
            case 8:
                Vec_Copy(p->pos, o->pos);
                break;
            case 10:
                switch (p->type) {
                case 0:
                    break;
                case 1:
                    p->pos[1] += p->unk38;
                    break;
                case 2:
                    p->pos[1] += p->unk38;
                    p->unk38 -= 3.0f;
                    break;
                }
                break;
            case 11:
                Mtx_GetTrans(o->pos, owner->weaponModel->partBones[0][3].world);
                break;
            case 12:
                switch (p->type) {
                case 1:
                    p->unk38 -= 2.5f;
                    p->pos[1] += p->unk38;
                    break;
                case 2:
                case 3:
                    p->unk38 -= 0.5f;
                    p->pos[1] += p->unk38;
                    break;
                }
                break;
            case 14:
                switch (p->type) {
                case 3:
                case 4:
                    p->pos[1] += p->unk38;
                    break;
                }
                break;
            case 15:
                p->pos[1] += 100.0f * o->scale;
                break;
            case 16:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 50.0f;
                Vec_RotateSelf((f32 *) v, gCamMatrix);
                p->pos[0] = o->pos[0] + v[0];
                p->pos[1] = o->pos[1] + v[1];
                p->pos[2] = o->pos[2] + v[2];
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
}

void StageObj_K29_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = gStageObjK29_Counts[o->type];
    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x24) != 0) {
            if (o->keep == 0) {
                DrawSlot_Free(*(s16 *) p);
            } else {
                DrawSlotB_Free(*(s16 *) p);
            }
        }
        p += 0x3C;
    }
}

void StageObj_K29_Free(StageObj *o)
{
    StageObj_Free(o);
}

/* Draws one part of a kind 0x29 stage object: the model from the type's table (or p->unk03 for
 * type 10's part type 1), then the order, mode and kind each type and part type select. pt caches
 * p->type for the part switches of types 1, 13 and 14, which come before any call; the other
 * types' switches read the field again. */
void StageObj_K29_Draw(DrawSlot *node)
{
    StageObj *o = node->owner;
    u8 kind;
    Obj29Part *p = (Obj29Part *) o->buf + node->part;
    u16 order = 0;
    u16 mode = 0;
    ModelMatSet *model;
    ModelMat *mats;
    u16 *tbl;
    u32 alpha;
    s32 k;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    f32 tm[16];
    f32 *sy;
    f32 *sz;
    f32 f;
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    u32 type;
    u8 pt;
    Model *set;

    kind = 1;

    if (o->visible && (type = o->type, tbl = StageObj_K29_DrawTbl[type], (set = gStageModelSet) != NULL) && set->active) {
        if (type == 10 && (pt = p->type) == 1) {
            model = &set->matSets[p->unk03];
        } else {
            pt = p->type;
            model = &set->matSets[tbl[pt]];
        }
        mats = set->mats;
        switch (type) {
        case 0:
        case 4:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            switch (p->type) {
            default:
            case 0:
            case 1:
            case 2:
                mode |= 2;
                order = 3;
                break;
            case 3:
                order = 3;
                break;
            case 4:
                order = 0;
                kind = 0;
                break;
            case 5:
                mode |= 2;
                order = 7;
                kind = 0;
                break;
            }
            break;
        case 1:
            switch (pt) {
            case 1:
                Mtx_Translation(tm, 0.75f, 0.25f, 0.0f);
                break;
            default:
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                break;
            }
            Gs_SetState(0x19, (s32) tm);
            order = 3;
            kind = 1;
            switch (p->type) {
            case 0:
            case 1:
                mode |= 2;
                break;
            case 3:
                mode |= 2;
                break;
            case 2:
                mode |= 4;
                break;
            case 4:
                mode |= 4;
                break;
            }
            break;
        case 2:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 2;
            switch (p->type) {
            case 0:
            case 5:
                order = 5;
                break;
            case 1:
            case 3:
            case 7:
            case 9:
                order = 3;
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                order = 4;
                kind = 3;
                break;
            default:
                return;
            }
            break;
        case 3:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 2;
            switch (p->type) {
            case 0:
                order = 3;
                break;
            case 1:
                order = 4;
                kind = 3;
                break;
            case 2:
                order = 7;
                kind = 0;
                break;
            default:
                return;
            }
            break;
        case 5:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 2;
            kind = 0;
            switch (p->type) {
            case 0:
                order = 1;
                break;
            case 1:
                break;
            default:
                return;
            }
            break;
        case 6:
        case 9:
        case 15:
            order = 3;
            break;
        case 7:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 4;
            order = 3;
            kind = 1;
            break;
        case 8:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 2;
            switch (p->type) {
            case 0:
            case 1:
                order = 5;
                break;
            case 2:
            case 3:
                order = 3;
                break;
            default:
                return;
            }
            break;
        case 10:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 4;
            Effect_GetColor(gScene.area, &r, &g, &b, &a);
            alpha = (u32) (((f32) (u32) a / 255.0f) * (f32) ((p->color >> 24) & 0xFF));
            p->color = b | ((g << 8) | ((r << 16) | ((alpha & 0xFF) << 24)));
            switch (p->type) {
            case 0:
                kind = 2;
                order = 3;
                break;
            case 1:
                kind = 1;
                order = 3;
                break;
            case 2:
                kind = 0;
                order = 3;
                break;
            default:
                return;
            }
            break;
        case 11:
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            mode |= 2;
            kind = 1;
            order = 3;
            break;
        case 12:
            kind = 1;
            order = 3;
            break;
        case 13:
            mode |= 2;
            kind = 2;
            switch (pt) {
            case 0:
                break;
            case 1:
                Mtx_Translation(tm, 0.25f * (p->unk03 & 1), 0.0625f * ((p->unk03 >> 1) & 1), 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            }
            break;
        case 14:
            switch (pt) {
            case 0:
                if (p->unk03 == 0) {
                    kind = 1;
                } else {
                    order = 1;
                    kind = 0;
                }
                mode |= 2;
                break;
            case 1:
                kind = 1;
                mode |= 2;
                break;
            case 2:
                kind = 0;
                mode |= 2;
                break;
            case 3:
            case 4:
                kind = 1;
                order = 3;
                break;
            default:
                return;
            }
            break;
        case 16:
            kind = 1;
            if (o->var1 == 0) {
                mode |= 2;
            } else {
                mode |= 8;
            }
            order = 3;
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        default:
            return;
        }
        k = kind;
        if (k == 3) {
            sy = &scale[1];
            sz = &scale[2];
            scale[0] = 1.0f;
            *sy = 1.0f;
            *sz = 1.0f;
        } else {
            sy = &scale[1];
            sz = &scale[2];
            scale[0] = p->sc[0] * p->scale;
            *sy = p->sc[1] * p->scale;
            *sz = p->sc[2] * p->scale;
        }
        rot[0] = (6.2831855f * (f32) (u32) p->unk2A) / 65536.0f;
        rot[1] = (6.2831855f * (f32) (u32) p->unk2C) / 65536.0f;
        rot[2] = (6.2831855f * (f32) (u32) p->unk2E) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
        switch (k) {
        case 0:
            break;
        case 1:
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 2:
            Mtx_Concat3x3(m, gCamYawMtx);
            break;
        case 3:
            Mtx_Billboard(m, node->pos);
            Mtx_TurnX(m, 1.5707964f);
            scale[0] = p->sc[0] * p->scale;
            *sy = p->sc[1] * p->scale;
            *sz = p->sc[2] * p->scale;
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
            break;
        }
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, m, p->color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K29_PlaySound(StageObj *o)
{
    switch (o->type) {
    case 0:
        Player_PlaySe(o->owner, 0xC, 0, ((Player *) o->owner)->pos, 1, 0);
        break;
    case 4:
        Player_PlaySe(o->owner, 0xE, 0, ((Player *) o->owner)->pos, 1, 0);
        break;
    case 8:
        Player_PlaySe(o->owner, 0x1C, 0, ((Player *) o->owner)->pos, 1, 0);
        break;
    }
}

s32 StageObj_K29_OwnerGone(void *obj)
{
    ObjHead *o = obj;

    if (o->active == 0 || o->unk004 == 3) {
        return 1;
    }
    return 0;
}

void StageObj_K29_InitPart(StageObj *o, Obj29Part *w, EmBone *bone, s16 i)
{
    Mtx_GetTrans(w->pos, bone->world);
    w->unk36 = 1;
    w->scale = o->scale;
    w->unk37 = 1;
    switch (w->type) {
    case 0:
    case 5:
        w->unk2E = Rand_Next(1);
        w->unk34 = 0xB6;
        if (i & 1) {
            w->unk2C = 0x8000;
        }
        break;
    case 1:
        w->unk2E = Rand_Next(1);
        w->unk38 = 70.0f / StageObj_K29_Frames1[w->type];
        break;
    case 3:
        w->scale *= StageObj_K29_Scale3[o->var2];
        break;
    case 4:
        w->unk28 = -2;
        w->scale *= StageObj_K29_Scale3[o->var2];
        break;
    case 7:
        w->scale *= 3.0f;
        break;
    case 8:
        w->unk28 = -2;
        w->scale *= 1.5f;
        break;
    case 9:
        w->unk28 = -4;
        w->unk2E = Rand_Next(1);
        break;
    }
}

/* the zero padding after the last jump table, to the end of the overlay .rodata (0x8F51E8-0x8F5200;
 * MWCC aligns an object of 0x18 bytes to 16, hence two) */
const u8 D_008F51E8[8] = {0};
const u8 D_008F51F0[0x10] = {0};

void StageObj_K29_RandomizeSpin(Obj29Part *w)
{
    w->unk28 = 1;
    w->scale = 0.8f + (0.7f * Lure_RandUnit());
    w->unk2E = Rand_Next(1);
    w->unk34 = (s16) (s32) (0.5f + ((65536.0f * ((360.0f * Lure_RandSigned()) / (f32) StageObj_K29_SpinDiv7[w->type])) / 360.0f));
}

void StageObj_K29_InitPartB(StageObj *o, Obj29Part *w, EmBone *bone, s16 i)
{
    Mtx_GetTrans(w->pos, bone->world);
    w->unk36 = 1;
    w->scale = o->scale;
    w->unk37 = 1;
    switch (w->type) {
    case 0:
        w->unk2E = Rand_Next(1);
        w->unk34 = 0x16C;
        if (i & 1) {
            w->unk2C = 0x8000;
        }
        break;
    case 1:
        w->unk2E = Rand_Next(1);
        w->unk34 = 0x7F;
        if (i & 1) {
            w->unk2C = 0x8000;
        }
        break;
    case 2:
        w->scale *= StageObj_K29_ScaleB2[o->var2];
        w->unk2E = Rand_Next(1);
        w->unk34 = 0;
        break;
    case 3:
        w->scale *= StageObj_K29_ScaleB2[o->var2];
        w->unk2E = 0;
        w->unk34 = 0;
        break;
    }
}

StageObj *StageObj_K29_Alloc(u8 k)
{
    return StageObj_Alloc(1);
}

void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale)
{
    StageObj_K29_CreateOnPlayer(p, k, 0, scale);
}

void StageObj_K29_CreateOnPlayer(Player *p, u8 k, s32 c, f32 scale)
{
    u8 local;
    s32 t;
    StageObj *o;
    EmBone *bone;
    f32 v[3];

    local = 0;
    t = k;
    if (t == 2 || t == 3 || t == 8 || t == 10) {
        if (!Player_IsLocal(p)) {
            return;
        }
    }
    if (t == 7) {
        local = 1;
    }
    if (local == 0 && !(Obj_InCurrentArea(p) & 0xFF)) {
        return;
    }
    if ((o = StageObj_K29_Alloc(k)) == NULL) {
        return;
    }
    o->kind = 0x29;
    o->update = StageObj_K29_Update;
    o->owner = p;
    o->type = k;
    o->scale = scale;
    o->var1 = c;
    if (local != 0) {
        o->keep = 1;
    }
    if (o->type == 3) {
        bone = Em_GetBone(p, 0x12);
        Vec_DirToAngles((f32 *) bone, (s16 *) &o->angle2, (s16 *) &o->angle);
        Mtx_GetTrans(o->pos, bone->world);
        v[0] = -110.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        Vec_RotateSelf((f32 *) v, (f32 *) bone);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
    }
}

void StageObj_K29_CreateAtDefault(f32 *src, u8 k, f32 scale)
{
    StageObj_K29_CreateAt(src, k, 0, scale);
}

void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale)
{
    StageObj *o = StageObj_K29_Alloc(k);

    if (o != NULL) {
        o->kind = 0x29;
        o->update = StageObj_K29_Update;
        o->owner = NULL;
        o->type = k;
        o->scale = scale;
        o->var1 = c;
        Vec_Copy(o->pos, src);
    }
}

void StageObj_K29_TickType6(Player *p, s32 flag)
{
    if ((u8) flag && !(gScene.frame & 0xF)) {
        StageObj_K29_CreateOnPlayer(p, 6, flag, 0.5f);
    }
}

void StageObj_K29_TickType9(Player *p)
{
    if (!(gScene.frame & 0xF)) {
        StageObj_K29_CreateOnPlayer(p, 9, 0, 1.0f);
    }
}

void StageObj_K29_TickType9Late(Player *p)
{
    if ((gScene.frame & 0xF) == 8) {
        StageObj_K29_CreateOnPlayer(p, 9, 1, 1.0f);
    }
}

void StageObj_BowArc_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_BowArc_Setup(o);
        break;
    case 1:
        StageObj_BowArc_Run(o);
        break;
    case 2:
        StageObj_BowArc_End(o);
        break;
    case 3:
        StageObj_BowArc_Free(o);
        break;
    }
}

void StageObj_BowArc_Setup(StageObj *o)
{
    Obj2BWork *w = (Obj2BWork *) o->buf;

    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    w->timer = 0;
    w->phase = 0;
    o->slot = DrawSlotB_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlotB_Get(o->slot);
        o->node->owner = o;
        o->node->draw = StageObj_BowArc_Draw;
    }
    StageObj_BowArc_Run(o);
}

void StageObj_BowArc_Run(StageObj *o)
{
    Obj2BWork *w;
    Em *em = o->owner;
    EmBone *bone;
    f32 dir[3];
    u16 yaw;
    u16 pitch;

    if (Bow_ArcVisible(em, 0) != 0) {
        w = (Obj2BWork *) o->buf;
        w->timer++;
        if (w->timer >= 0x18) {
            w->phase++;
            w->timer = 0;
            if (w->phase >= 2) {
                w->phase = 0;
            }
        }
        dir[0] = 1.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
        bone = Em_GetBone(em, 0xE);
        Vec_RotateSelf((f32 *) dir, (f32 *) bone);
        Mtx_GetTrans(o->pos, bone->world);
        Vec_DirToAngles(dir, (s16 *) &yaw, (s16 *) &pitch);
        w->yaw = yaw;
        w->pitch = pitch;
        w->unk08 = 0;
        o->node->pos[0] = o->pos[0];
        o->node->pos[1] = o->pos[1];
        o->node->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
    }
}

void StageObj_BowArc_End(StageObj *o)
{
    o->state++;
    if (o->node != NULL) {
        DrawSlotB_Free(o->slot);
    }
}

void StageObj_BowArc_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_BowArc_Draw(DrawSlot *node)
{
    s16 count;
    f32 grav;
    f32 pos[3];
    f32 vel[3];
    f32 m[16];
    f32 sm[16];
    StageObj *o = node->owner;
    Player *pl = o->owner;
    Obj2BWork *w = (Obj2BWork *) o->buf;
    s32 i;
    s32 k;
    Model *set;
    ModelMatSet *model;
    u8 *bones;
    ModelMat *tex;
    u8 r;
    u8 g;

    if (pl->weaponType != 10 || pl->active == 0 || pl->visible == 0) {
        return;
    }
    if (gScene.area == pl->area && Player_IsLocal(pl)) {
        set = gStageModelSets[2];
        if (set != NULL && set->active != 0) {
            Gs_SetState(0x60, 0);
            Gfx_SetState63(1);
            tex = set->mats;
            model = set->matSets;
            Mtx_Scale(sm, 1.75f, 1.75f, 1.0f);
            BowgunShot_FlightParams(0, (s32 *) vel, &grav, &count);
            Mtx_Identity(m);
            Mtx_RotateX(m, (6.2831855f * w->yaw) / 65536.0f);
            Mtx_RotateY(m, (6.2831855f * w->pitch) / 65536.0f);
            Vec_RotateSelf((f32 *) vel, (f32 *) m);
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Mtx_Concat3x3(m, sm);
            Bone_UpdateWorld(set->bones, m);
            bones = (u8 *)set->bones;
            Vec_Copy(pos, o->pos);
            k = 2;
            for (i = 0; i < 32; i++) {
                if (!(i & 1)) {
                    Mtx_SetTrans((f32 *) (bones + k * 0x190), pos[0], pos[1], pos[2]);
                    k++;
                    if (k >= 18) {
                        break;
                    }
                }
                pos[0] += vel[0];
                pos[1] += vel[1];
                pos[2] += vel[2];
                if (count <= 0) {
                    vel[1] -= grav;
                } else {
                    count--;
                }
            }
            r = 0x40;
            g = r;
            switch (w->phase) {
            case 0:
                r = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xA0;
                g = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xA0;
                break;
            case 1:
                r = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xB0;
                break;
            }
            Bone_StoreMatrices(set->bones);
            Gs_SetState(0x67, (r << 16) | 0xC0000000 | (g << 8) | 0x40);
            for (i = 0; i < set->matSetCount; i++) {
                if (model != NULL) {
                    if (model->id != -1) {
                        Gs_SetLightList(tex, model);
                        Draw_ApplyStates(model->flags);
                        Model_Draw(model->id, 0);
                    }
                    model++;
                }
            }
            Draw_SetDefaultState();
        }
    }
}

/* A bow arc (kind 0x2B) that follows owner; kept when the stage objects are freed. */
void StageObj_BowArc_Create(void *owner)
{
    StageObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x2B;
        o->update = StageObj_BowArc_Update;
        o->owner = owner;
        o->keep = 1;
    }
}

/* unk_88E760's .data (0x8D8B90-0x8DA570) */
extern u32 D_008F5CE8[];
extern u32 __start[];
f32 StageObj_K25_Offset[4] = {
#include "game/008D8B90.inc"
};
u16 StageObj_K25_Fade[4] = {
#include "game/008D8BA0.inc"
};
u8 StageObj_K27_ModelSetIds[8] = {
#include "game/008D8BA8.inc"
};
u8 StageObj_K27_ScaleKeys5[64] = {
#include "game/008D8BB0.inc"
};
u8 StageObj_K27_ColorKeys5[32] = {
#include "game/008D8BF0.inc"
};
u8 StageObj_K27_ScaleKeys9[64] = {
#include "game/008D8C10.inc"
};
u8 StageObj_K27_ColorKeys9[48] = {
#include "game/008D8C50.inc"
};
Vec3f StageObj_K27_Vecs9[4] = {
#include "game/008D8C80.inc"
};
u32 StageObj_K29_Curves_0[8] = {
#include "game/008D8CB0.inc"
};
u32 StageObj_K29_Curves_1[8] = {
#include "game/008D8CD0.inc"
};
u32 StageObj_K29_Curves_2[12] = {
#include "game/008D8CF0.inc"
};
u32 StageObj_K29_Curves_3[8] = {
#include "game/008D8D20.inc"
};
u32 StageObj_K29_Curves_4[12] = {
#include "game/008D8D40.inc"
};
u32 StageObj_K29_Curves_5[8] = {
#include "game/008D8D70.inc"
};
u8 StageObj_K29_ColorKeys_0[32] = {
#include "game/008D8D90.inc"
};
u8 StageObj_K29_ColorKeys_1[32] = {
#include "game/008D8DB0.inc"
};
u8 StageObj_K29_ColorKeys_2[32] = {
#include "game/008D8DD0.inc"
};
u8 StageObj_K29_ColorKeys_3[32] = {
#include "game/008D8DF0.inc"
};
u8 StageObj_K29_ColorKeys_4[32] = {
#include "game/008D8E10.inc"
};
u8 StageObj_K29_ColorKeys_5[24] = {
#include "game/008D8E30.inc"
};
u16 StageObj_K29_DrawTbl_0[6] = {
#include "game/008D8E48.inc"
};
s16 StageObj_K29_SpinFrames7[6] = {
#include "game/008D8E58.inc"
};
s16 StageObj_K29_Delay0[8] = {
#include "game/008D8E70.inc"
};
u32 StageObj_K29_Curves_6[12] = {
#include "game/008D8E80.inc"
};
u32 StageObj_K29_Curves_7[8] = {
#include "game/008D8EB0.inc"
};
u32 StageObj_K29_Curves_8[8] = {
#include "game/008D8ED0.inc"
};
u32 StageObj_K29_Curves_9[8] = {
#include "game/008D8EF0.inc"
};
u32 StageObj_K29_Curves_10[12] = {
#include "game/008D8F10.inc"
};
u8 StageObj_K29_ColorKeys_6[32] = {
#include "game/008D8F40.inc"
};
u8 StageObj_K29_ColorKeys_7[32] = {
#include "game/008D8F60.inc"
};
u8 StageObj_K29_ColorKeys_8[48] = {
#include "game/008D8F80.inc"
};
u8 StageObj_K29_ColorKeys_9[32] = {
#include "game/008D8FB0.inc"
};
u8 StageObj_K29_ColorKeys_10[72] = {
#include "game/008D8FD0.inc"
};
f32 StageObj_K29_Vec2[3] = {
#include "game/008D9018.inc"
};
u32 StageObj_K29_Curves_11[16] = {
#include "game/008D9030.inc"
};
u32 StageObj_K29_Curves_12[8] = {
#include "game/008D9070.inc"
};
u32 StageObj_K29_Curves_13[12] = {
#include "game/008D9090.inc"
};
u32 StageObj_K29_Curves_14[8] = {
#include "game/008D90C0.inc"
};
u32 StageObj_K29_Curves_15[8] = {
#include "game/008D90E0.inc"
};
u32 StageObj_K29_Curves_16[12] = {
#include "game/008D9100.inc"
};
u32 StageObj_K29_Curves_17[12] = {
#include "game/008D9130.inc"
};
u32 StageObj_K29_Curves_18[8] = {
#include "game/008D9160.inc"
};
u32 StageObj_K29_Curves_19[8] = {
#include "game/008D9180.inc"
};
u32 StageObj_K29_Curves_20[8] = {
#include "game/008D91A0.inc"
};
u8 StageObj_K29_ColorKeys_11[32] = {
#include "game/008D91C0.inc"
};
u8 StageObj_K29_ColorKeys_12[32] = {
#include "game/008D91E0.inc"
};
u8 StageObj_K29_ColorKeys_13[32] = {
#include "game/008D9200.inc"
};
u8 StageObj_K29_ColorKeys_14[32] = {
#include "game/008D9220.inc"
};
u8 StageObj_K29_ColorKeys_15[32] = {
#include "game/008D9240.inc"
};
u8 StageObj_K29_ColorKeys_16[32] = {
#include "game/008D9260.inc"
};
u8 StageObj_K29_ColorKeys_17[32] = {
#include "game/008D9280.inc"
};
u8 StageObj_K29_ColorKeys_18[32] = {
#include "game/008D92A0.inc"
};
u8 StageObj_K29_ColorKeys_19[32] = {
#include "game/008D92C0.inc"
};
u8 StageObj_K29_ColorKeys_20[24] = {
#include "game/008D92E0.inc"
};
u8 StageObj_K29_Types2[8] = {
#include "game/008D92F8.inc"
};
u8 StageObj_K29_Kinds3[16] = {
#include "game/008D9300.inc"
};
u16 StageObj_K29_DrawTbl_2[16] = {
#include "game/008D9310.inc"
};
s16 StageObj_K29_Frames1[12] = {
#include "game/008D9330.inc"
};
u8 StageObj_K29_Alpha2[8] = {
#include "game/008D9348.inc"
};
f32 StageObj_K29_Scale3[4] = {
#include "game/008D9350.inc"
};
u32 StageObj_K29_Curves_21[8] = {
#include "game/008D9360.inc"
};
u32 StageObj_K29_Curves_22[8] = {
#include "game/008D9380.inc"
};
u32 StageObj_K29_Curves_23[12] = {
#include "game/008D93A0.inc"
};
u8 StageObj_K29_ColorKeys_21[32] = {
#include "game/008D93D0.inc"
};
u8 StageObj_K29_ColorKeys_22[32] = {
#include "game/008D93F0.inc"
};
u8 StageObj_K29_ColorKeys_23[24] = {
#include "game/008D9410.inc"
};
u8 StageObj_K29_Types3[8] = {
#include "game/008D9428.inc"
};
u16 StageObj_K29_Delay3[8] = {
#include "game/008D9430.inc"
};
u16 StageObj_K29_DrawTbl_3[4] = {
#include "game/008D9440.inc"
};
s16 StageObj_K29_SpinFrames2[4] = {
#include "game/008D9448.inc"
};
u32 StageObj_K29_Curves_24[12] = {
#include "game/008D9450.inc"
};
u32 StageObj_K29_Curves_25[16] = {
#include "game/008D9480.inc"
};
u8 StageObj_K29_ColorKeys_24[32] = {
#include "game/008D94C0.inc"
};
u8 StageObj_K29_ColorKeys_25[32] = {
#include "game/008D94E0.inc"
};
u8 StageObj_K29_Types5[8] = {
#include "game/008D9500.inc"
};
u16 StageObj_K29_Delay5[4] = {
#include "game/008D9508.inc"
};
u16 StageObj_K29_DrawTbl_5[4] = {
#include "game/008D9510.inc"
};
s16 StageObj_K29_Durations_5[4] = {
#include "game/008D9518.inc"
};
u32 StageObj_K29_Curves_26[8] = {
#include "game/008D9520.inc"
};
u8 StageObj_K29_ColorKeys_26[48] = {
#include "game/008D9540.inc"
};
u8 StageObj_K29_ColorKeys_27[40] = {
#include "game/008D9570.inc"
};
u16 StageObj_K29_DrawTbl_6[4] = {
#include "game/008D9598.inc"
};
s16 StageObj_K29_Durations_6[8] = {
#include "game/008D95A0.inc"
};
u32 StageObj_K29_Curves_27[12] = {
#include "game/008D95B0.inc"
};
u8 StageObj_K29_ColorKeys_28[32] = {
#include "game/008D95E0.inc"
};
s16 StageObj_K29_Delay7[4] = {
#include "game/008D9600.inc"
};
s16 StageObj_K29_SpinDiv7[4] = {
#include "game/008D9608.inc"
};
u16 StageObj_K29_DrawTbl_7[8] = {
#include "game/008D9610.inc"
};
u32 StageObj_K29_Curves_28[12] = {
#include "game/008D9620.inc"
};
u32 StageObj_K29_Curves_29[8] = {
#include "game/008D9650.inc"
};
u32 StageObj_K29_Curves_30[12] = {
#include "game/008D9670.inc"
};
u32 StageObj_K29_Curves_31[12] = {
#include "game/008D96A0.inc"
};
u8 StageObj_K29_ColorKeys_29[32] = {
#include "game/008D96D0.inc"
};
u8 StageObj_K29_ColorKeys_30[32] = {
#include "game/008D96F0.inc"
};
u8 StageObj_K29_ColorKeys_31[32] = {
#include "game/008D9710.inc"
};
u8 StageObj_K29_ColorKeys_32[32] = {
#include "game/008D9730.inc"
};
u8 StageObj_K29_Types8[8] = {
#include "game/008D9750.inc"
};
u16 StageObj_K29_DrawTbl_8[4] = {
#include "game/008D9758.inc"
};
s16 StageObj_K29_Param28[4] = {
#include "game/008D9760.inc"
};
f32 StageObj_K29_ScaleB2[3] = {
#include "game/008D9768.inc"
};
u8 StageObj_K29_ColorKeys_33[48] = {
#include "game/008D9780.inc"
};
u8 StageObj_K29_ColorKeys_34[40] = {
#include "game/008D97B0.inc"
};
u16 StageObj_K29_DrawTbl_10[4] = {
#include "game/008D97D8.inc"
};
u8 StageObj_K29_Unk03Pick[16] = {
#include "game/008D97E0.inc"
};
u32 StageObj_K29_Curves_32[12] = {
#include "game/008D97F0.inc"
};
u32 StageObj_K29_Curves_33[12] = {
#include "game/008D9820.inc"
};
u32 StageObj_K29_Curves_34[16] = {
#include "game/008D9850.inc"
};
u8 StageObj_K29_ColorKeys_35[32] = {
#include "game/008D9890.inc"
};
u8 StageObj_K29_ColorKeys_36[32] = {
#include "game/008D98B0.inc"
};
u8 StageObj_K29_ColorKeys_37[24] = {
#include "game/008D98D0.inc"
};
f32 StageObj_K29_Depth10[3] = {
#include "game/008D98E8.inc"
};
u8 StageObj_K29_Types10[12] = {
#include "game/008D98F8.inc"
};
u16 StageObj_K29_Delay10[12] = {
#include "game/008D9910.inc"
};
s16 StageObj_K29_Frames1B[4] = {
#include "game/008D9928.inc"
};
u16 StageObj_K29_DrawTbl_11[8] = {
#include "game/008D9930.inc"
};
u32 StageObj_K29_Curves_35[12] = {
#include "game/008D9940.inc"
};
u32 StageObj_K29_Curves_36[12] = {
#include "game/008D9970.inc"
};
u8 StageObj_K29_ColorKeys_38[32] = {
#include "game/008D99A0.inc"
};
u8 StageObj_K29_ColorKeys_39[32] = {
#include "game/008D99C0.inc"
};
u8 StageObj_K29_ColorKeys_40[32] = {
#include "game/008D99E0.inc"
};
u8 StageObj_K29_ColorKeys_41[32] = {
#include "game/008D9A00.inc"
};
u8 StageObj_K29_ColorKeys_42[32] = {
#include "game/008D9A20.inc"
};
u8 StageObj_K29_ColorKeys_43[32] = {
#include "game/008D9A40.inc"
};
u8 StageObj_K29_ColorKeys_44[32] = {
#include "game/008D9A60.inc"
};
u8 StageObj_K29_ColorKeys_45[32] = {
#include "game/008D9A80.inc"
};
u8 StageObj_K29_ColorKeys_46[32] = {
#include "game/008D9AA0.inc"
};
u8 StageObj_K29_ColorKeys_47[32] = {
#include "game/008D9AC0.inc"
};
u8 StageObj_K29_ColorKeys_48[32] = {
#include "game/008D9AE0.inc"
};
u8 StageObj_K29_ColorKeys_49[32] = {
#include "game/008D9B00.inc"
};
u8 StageObj_K29_Types11[8] = {
#include "game/008D9B20.inc"
};
s16 StageObj_K29_Durations_11[4] = {
#include "game/008D9B28.inc"
};
s16 StageObj_K29_Delay11[4] = {
#include "game/008D9B30.inc"
};
u16 StageObj_K29_DrawTbl_12[4] = {
#include "game/008D9B38.inc"
};
u32 StageObj_K29_Curves_37[8] = {
#include "game/008D9B40.inc"
};
u32 StageObj_K29_Curves_38[12] = {
#include "game/008D9B60.inc"
};
u32 StageObj_K29_Curves_39[12] = {
#include "game/008D9B90.inc"
};
u8 StageObj_K29_ColorKeys_50[32] = {
#include "game/008D9BC0.inc"
};
u8 StageObj_K29_ColorKeys_51[32] = {
#include "game/008D9BE0.inc"
};
u8 StageObj_K29_ColorKeys_52[32] = {
#include "game/008D9C00.inc"
};
u8 StageObj_K29_Types12[8] = {
#include "game/008D9C20.inc"
};
s16 StageObj_K29_Delay12[4] = {
#include "game/008D9C28.inc"
};
s16 StageObj_K29_Durations_12[4] = {
#include "game/008D9C30.inc"
};
u16 StageObj_K29_DrawTbl_13[4] = {
#include "game/008D9C38.inc"
};
u32 StageObj_K29_Curves_41[16] = {
#include "game/008D9C40.inc"
};
u32 StageObj_K29_Curves_42[12] = {
#include "game/008D9C80.inc"
};
u8 StageObj_K29_ColorKeys_54[32] = {
#include "game/008D9CB0.inc"
};
u8 StageObj_K29_ColorKeys_55[40] = {
#include "game/008D9CD0.inc"
};
s16 StageObj_K29_Delay13[4] = {
#include "game/008D9CF8.inc"
};
u32 StageObj_K29_Durations_13[2] = {
    (u32)__start, 0,
};
u16 StageObj_K29_DrawTbl_14[6] = {
#include "game/008D9D08.inc"
};
u32 StageObj_K29_Curves_43[12] = {
#include "game/008D9D20.inc"
};
u32 StageObj_K29_Curves_44[8] = {
#include "game/008D9D50.inc"
};
u32 StageObj_K29_Curves_45[12] = {
#include "game/008D9D70.inc"
};
u32 StageObj_K29_Curves_46[8] = {
#include "game/008D9DA0.inc"
};
u8 StageObj_K29_ColorKeys_56[48] = {
#include "game/008D9DC0.inc"
};
u8 StageObj_K29_ColorKeys_57[32] = {
#include "game/008D9DF0.inc"
};
u8 StageObj_K29_ColorKeys_58[32] = {
#include "game/008D9E10.inc"
};
u8 StageObj_K29_ColorKeys_59[32] = {
#include "game/008D9E30.inc"
};
u8 StageObj_K29_Types14[24] = {
#include "game/008D9E50.inc"
};
s16 StageObj_K29_Durations_14[6] = {
#include "game/008D9E68.inc"
};
u32 StageObj_K29_Curves_48[8] = {
#include "game/008D9E80.inc"
};
u8 StageObj_K29_ColorKeys_61[32] = {
#include "game/008D9EA0.inc"
};
u16 StageObj_K29_DrawTbl_15[4] = {
#include "game/008D9EC0.inc"
};
s16 StageObj_K29_Durations_15[4] = {
#include "game/008D9EC8.inc"
};
u32 StageObj_K29_Curves_50[12] = {
#include "game/008D9ED0.inc"
};
u32 StageObj_K29_Curves_51[12] = {
#include "game/008D9F00.inc"
};
u8 StageObj_K29_ColorKeys_63[32] = {
#include "game/008D9F30.inc"
};
u8 StageObj_K29_ColorKeys_64[32] = {
#include "game/008D9F50.inc"
};
s16 StageObj_K29_Durations_16[8] = {
#include "game/008D9F70.inc"
};
void *StageObj_K29_Curves[52] = {
    (void *)StageObj_K29_Curves_0, (void *)StageObj_K29_Curves_1, (void *)StageObj_K29_Curves_2, (void *)StageObj_K29_Curves_3,
    (void *)StageObj_K29_Curves_4, (void *)StageObj_K29_Curves_5, (void *)StageObj_K29_Curves_6, (void *)StageObj_K29_Curves_7,
    (void *)StageObj_K29_Curves_8, (void *)StageObj_K29_Curves_9, (void *)StageObj_K29_Curves_10, (void *)StageObj_K29_Curves_11,
    (void *)StageObj_K29_Curves_12, (void *)StageObj_K29_Curves_13, (void *)StageObj_K29_Curves_14, (void *)StageObj_K29_Curves_15,
    (void *)StageObj_K29_Curves_16, (void *)StageObj_K29_Curves_17, (void *)StageObj_K29_Curves_18, (void *)StageObj_K29_Curves_19,
    (void *)StageObj_K29_Curves_20, (void *)StageObj_K29_Curves_21, (void *)StageObj_K29_Curves_22, (void *)StageObj_K29_Curves_23,
    (void *)StageObj_K29_Curves_24, (void *)StageObj_K29_Curves_25, (void *)StageObj_K29_Curves_26, (void *)StageObj_K29_Curves_27,
    (void *)StageObj_K29_Curves_28, (void *)StageObj_K29_Curves_29, (void *)StageObj_K29_Curves_30, (void *)StageObj_K29_Curves_31,
    (void *)StageObj_K29_Curves_32, (void *)StageObj_K29_Curves_33, (void *)StageObj_K29_Curves_34, (void *)StageObj_K29_Curves_35,
    (void *)StageObj_K29_Curves_36, (void *)StageObj_K29_Curves_37, (void *)StageObj_K29_Curves_38, (void *)StageObj_K29_Curves_39,
    (void *)StageObj_K29_Curves_39, (void *)StageObj_K29_Curves_41, (void *)StageObj_K29_Curves_42, (void *)StageObj_K29_Curves_43,
    (void *)StageObj_K29_Curves_44, (void *)StageObj_K29_Curves_45, (void *)StageObj_K29_Curves_46, (void *)StageObj_K29_Curves_46,
    (void *)StageObj_K29_Curves_48, (void *)StageObj_K29_Curves_48, (void *)StageObj_K29_Curves_50, (void *)StageObj_K29_Curves_51,
};
u8 *StageObj_K29_ColorKeys[68] = {
    StageObj_K29_ColorKeys_0, StageObj_K29_ColorKeys_1, StageObj_K29_ColorKeys_2, StageObj_K29_ColorKeys_3,
    StageObj_K29_ColorKeys_4, StageObj_K29_ColorKeys_5, StageObj_K29_ColorKeys_6, StageObj_K29_ColorKeys_7,
    StageObj_K29_ColorKeys_8, StageObj_K29_ColorKeys_9, StageObj_K29_ColorKeys_10, StageObj_K29_ColorKeys_11,
    StageObj_K29_ColorKeys_12, StageObj_K29_ColorKeys_13, StageObj_K29_ColorKeys_14, StageObj_K29_ColorKeys_15,
    StageObj_K29_ColorKeys_16, StageObj_K29_ColorKeys_17, StageObj_K29_ColorKeys_18, StageObj_K29_ColorKeys_19,
    StageObj_K29_ColorKeys_20, StageObj_K29_ColorKeys_21, StageObj_K29_ColorKeys_22, StageObj_K29_ColorKeys_23,
    StageObj_K29_ColorKeys_24, StageObj_K29_ColorKeys_25, StageObj_K29_ColorKeys_26, StageObj_K29_ColorKeys_27,
    StageObj_K29_ColorKeys_28, StageObj_K29_ColorKeys_29, StageObj_K29_ColorKeys_30, StageObj_K29_ColorKeys_31,
    StageObj_K29_ColorKeys_32, StageObj_K29_ColorKeys_33, StageObj_K29_ColorKeys_34, StageObj_K29_ColorKeys_35,
    StageObj_K29_ColorKeys_36, StageObj_K29_ColorKeys_37, StageObj_K29_ColorKeys_38, StageObj_K29_ColorKeys_39,
    StageObj_K29_ColorKeys_40, StageObj_K29_ColorKeys_41, StageObj_K29_ColorKeys_42, StageObj_K29_ColorKeys_43,
    StageObj_K29_ColorKeys_44, StageObj_K29_ColorKeys_45, StageObj_K29_ColorKeys_46, StageObj_K29_ColorKeys_47,
    StageObj_K29_ColorKeys_48, StageObj_K29_ColorKeys_49, StageObj_K29_ColorKeys_50, StageObj_K29_ColorKeys_51,
    StageObj_K29_ColorKeys_52, StageObj_K29_ColorKeys_52, StageObj_K29_ColorKeys_54, StageObj_K29_ColorKeys_55,
    StageObj_K29_ColorKeys_56, StageObj_K29_ColorKeys_57, StageObj_K29_ColorKeys_58, StageObj_K29_ColorKeys_59,
    StageObj_K29_ColorKeys_59, StageObj_K29_ColorKeys_61, StageObj_K29_ColorKeys_61, StageObj_K29_ColorKeys_63,
    StageObj_K29_ColorKeys_64, NULL, NULL, NULL,
};
s16 gStageObjK29_Counts[24] = {
#include "game/008DA160.inc"
};
s16 StageObj_K29_Life[24] = {
#include "game/008DA190.inc"
};
s16 StageObj_K29_CurveIdx[24] = {
#include "game/008DA1C0.inc"
};
s16 StageObj_K29_ColorIdx[24] = {
#include "game/008DA1F0.inc"
};
u16 *StageObj_K29_DrawTbl[20] = {
    StageObj_K29_DrawTbl_0, (u16 *)&StageObj_K29_ColorKeys_10[24], StageObj_K29_DrawTbl_2, StageObj_K29_DrawTbl_3,
    StageObj_K29_DrawTbl_0, StageObj_K29_DrawTbl_5, StageObj_K29_DrawTbl_6, StageObj_K29_DrawTbl_7,
    StageObj_K29_DrawTbl_8, StageObj_K29_DrawTbl_6, StageObj_K29_DrawTbl_10, StageObj_K29_DrawTbl_11,
    StageObj_K29_DrawTbl_12, StageObj_K29_DrawTbl_13, StageObj_K29_DrawTbl_14, StageObj_K29_DrawTbl_15,
    (u16 *)D_008F5CE8, NULL, NULL, NULL,
};
s16 *StageObj_K29_Durations[36] = {
    StageObj_K29_SpinFrames7, (s16 *)&StageObj_K29_ColorKeys_10[40], StageObj_K29_Frames1, StageObj_K29_SpinFrames2,
    StageObj_K29_SpinFrames7, StageObj_K29_Durations_5, StageObj_K29_Durations_6, StageObj_K29_SpinDiv7,
    StageObj_K29_Param28, StageObj_K29_Durations_6, StageObj_K29_Frames1B, StageObj_K29_Durations_11,
    StageObj_K29_Durations_12, (s16 *)StageObj_K29_Durations_13, StageObj_K29_Durations_14, StageObj_K29_Durations_15,
    StageObj_K29_Durations_16, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_008DA300[8] = {
#include "game/008DA300.inc"
};
u32 D_008DA320[8] = {
#include "game/008DA320.inc"
};
u32 D_008DA340[12] = {
#include "game/008DA340.inc"
};
u32 D_008DA370[12] = {
#include "game/008DA370.inc"
};
u32 D_008DA3A0[12] = {
#include "game/008DA3A0.inc"
};
u32 D_008DA3D0[8] = {
#include "game/008DA3D0.inc"
};
u32 D_008DA3F0[8] = {
#include "game/008DA3F0.inc"
};
u32 D_008DA410[8] = {
#include "game/008DA410.inc"
};
u32 D_008DA430[12] = {
#include "game/008DA430.inc"
};
u32 D_008DA460[12] = {
#include "game/008DA460.inc"
};
u32 D_008DA490[8] = {
#include "game/008DA490.inc"
};
u32 D_008DA4B0[8] = {
#include "game/008DA4B0.inc"
};
u32 D_008DA4D0[12] = {
#include "game/008DA4D0.inc"
};
u32 D_008DA500[8] = {
#include "game/008DA500.inc"
};
u32 D_008DA520[12] = {
#include "game/008DA520.inc"
};
u32 D_008DA550[8] = {
#include "game/008DA550.inc"
};
