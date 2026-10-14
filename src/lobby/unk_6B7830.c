/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
/* Level 4: every function matches there, and VObj_Run is closer. */
#pragma optimization_level 4
void DrawSlot_Free(s32);

/* prototypes */
void ModelObj_Free(VObj *o);
void VObj_Init(VObj *o);
void VObj_Run(VObj *o);
void VObj_End(VObj *o);
void VObj_Free(VObj *o);
/* end of prototypes */


void ModelObj_Free(VObj *o)
{
    StageObj_Free(o);
}

/* A part of a village object drawn as a model (0x34 bytes; ModelObj_DrawPart). */
typedef struct VModelPart {
    /* 0x00 */ s16 slot;
    /* 0x02 */ s16 kind;
    /* 0x04 */ Vec3f size;
    /* 0x10 */ Vec3f pos;
    /* 0x1C */ f32 alpha;
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ u16 ang;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ f32 scale;
    /* 0x2C */ DrawSlot *draw;
    /* 0x30 */ s16 unk30;
    /* 0x32 */ u16 unk32;
} VModelPart;

s16 Effect_MapTexId(s16 id);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8, u32);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
s32 Cook_InBestWindow(void);
extern f32 gCamMatrix[];
extern s16 D_007DCA88[];
extern u8 gModelObjColorKeys[];
extern u8 gModelObjPartColors[];
extern u8 D_007DC5F1[];
extern u8 D_007DC5F2[];
extern u8 D_007DC5F3[];

/* Draws a model part of a village object (DrawSlot callback). */
void ModelObj_DrawPart(DrawSlot *slot)
{
    s32 color;
    Vec3f scale;
    Vec3f rot;
    f32 m[4][4];
    f32 t[4][4];
    u8 cr;
    u8 cg;
    u8 cb;
    ModelMatSet *mdl;
    VObj *o = slot->owner;
    Model *set = gStageModelSet;
    VModelPart *p = &((VModelPart *)o->buf)[slot->part];
    u16 flags = 0;
    s32 order;
    s16 id;
    ModelMat *tex;
    s32 k;

    if (set != NULL && set->active != 0) {
        tex = set->mats;
        scale.x = p->scale * p->size.x;
        scale.y = p->scale * p->size.y;
        scale.z = p->scale * p->size.z;
        switch (o->type) {
        case 0:
            cb = cg = cr = 0xFF;
            id = Effect_MapTexId(D_007DCA88[p->kind]);
            switch (p->kind) {
            case 0:
                order = 0;
                break;
            case 1:
                flags |= 2;
                order = 0;
                cg = 0xEA;
                cb = 0x94;
                break;
            case 2:
                flags |= 2;
                order = 3;
                cg = 0x7C;
                cb = 0x76;
                break;
            case 3:
                flags |= 2;
                rot.z = 6.2831855f * p->ang / 65536.0f;
                order = 3;
                cr = 0xFF;
                cg = 0x39;
                cb = 0x76;
                break;
            case 4:
            case 5:
            case 6:
                rot.z = 6.2831855f * p->ang / 65536.0f;
                order = 3;
                break;
            }
            color = ((u8)(255.0f * p->alpha) << 24) | (cr << 16) | (cg << 8) | cb;
            break;
        case 1:
            id = Effect_MapTexId(0x4A);
            order = 0;
            Color_EvalKeys((s32 *) gModelObjColorKeys, p->unk30, (u32 *) &color);
            break;
        case 2:
            id = Effect_MapTexId(0x48);
            rot.z = 6.2831855f * p->ang / 65536.0f;
            order = 3;
            k = o->var2 * 4;
            color = ((u8)(D_007DC5F3[k] * p->alpha) << 24) | (gModelObjPartColors[k] << 16) |
                    (D_007DC5F1[k] << 8) | D_007DC5F2[k];
            break;
        case 6:
            switch (p->kind) {
            case 0:
            case 1:
            case 2:
            case 3:
                id = Effect_MapTexId(0x40);
                break;
            case 4:
                id = Effect_MapTexId(0x25);
                Mtx_Translation(t[0], 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (u32)t);
                break;
            case 5:
            case 6:
            default:
                id = Effect_MapTexId(0x58);
                break;
            }
            flags |= 2;
            rot.z = 6.2831855f * p->ang / 65536.0f;
            order = 3;
            color = ((u8)(255.0f * p->alpha) << 24) | 0x7FDFFF;
            break;
        case 7:
            id = Effect_MapTexId(0x25);
            flags |= 2;
            rot.z = 6.2831855f * p->ang / 65536.0f;
            order = 3;
            color = -1;
            Mtx_Translation(t[0], 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (u32)t);
            break;
        case 8:
            id = Effect_MapTexId(0x17);
            order = 0;
            if (Cook_InBestWindow() == 1) {
                flags |= 2;
                color = -1;
            } else {
                color = -1;
            }
            break;
        }
        if (id != -1) {
            mdl = &set->matSets[id];
            Mtx_FromScaleRotTrans(&scale.x, &rot.x, slot->pos, order, m[0]);
            Mtx_Concat3x3(m[0], gCamMatrix);
            Effect_DrawModel(mdl, m[0], color, flags, tex);
        }
    }
}

void VObj_Update(VObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        VObj_Init(o);
        break;
    case 1:
        VObj_Run(o);
        break;
    case 2:
        VObj_End(o);
        break;
    case 3:
        VObj_Free(o);
        break;
    }
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u16 Rand_Next(s32 stream);
f32 Lure_RandUnit(void);
u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
void VObj_Draw(DrawSlot *);
extern f32 gCamMatrix[];
extern Vec3f gVObjRowBases[];
extern s8 D_007DCDD4[];
extern s8 D_007DCDD9[];

/* Init (state 0): kind 0 lays its 15 members out in 3 rows of 5 along the
 * camera x axis and gives each a draw slot; the other kinds set a timer and
 * a turn rate and take one slot for the object. */
void VObj_Init(VObj *o)
{
    VObjData *d = (VObjData *)o->buf;
    u8 *kinds;
    Vec3f dir;
    u8 i;
    u8 col;
    s8 sign;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->state = 1;
    o->visible = 1;
    o->draw = 0;
    o->timer = 0;
    if (o->type == 0) {
        kinds = d->kinds;
        d->spin = 0x800;
        d->unk154 = 12;
        d->unk155 = 0;
        d->finished = 0;
        d->heading[15] = 0;
        dir.x = -500.0f;
        dir.y = 0.0f;
        dir.z = 0.0f;
        Vec_Rotate(&dir.x, &dir.x, gCamMatrix);
        dir.x *= 0.2f;
        dir.y *= 0.2f;
        dir.z *= 0.2f;
        for (i = 0; i < 15; i++) {
            d->flag[i] = 0;
            d->unk111[i] = 0;
            d->timer[i] = gVObjKinds[kinds[i]].timer;
            d->ang[i] = 0xA000;
            d->heading[i] = Math_Heading(d->pos.x, d->pos.z, 0.0f, 0.0f) + 0x8000;
            d->unk120[i] = 0;
            col = i % 5;
            d->spots[i].x = gVObjRowBases[(u8)(i / 5)].x + dir.x * col;
            d->spots[i].y = gVObjRowBases[(u8)(i / 5)].y + dir.y * col;
            d->spots[i].z = gVObjRowBases[(u8)(i / 5)].z + dir.z * col;
            d->slot[i] = DrawSlot_Alloc();
            if (d->slot[i] != -1) {
                d->parts[i] = DrawSlot_Get(d->slot[i]);
                d->parts[i]->owner = o;
                d->parts[i]->part = i;
                d->parts[i]->draw = VObj_Draw;
            } else {
                d->parts[i] = NULL;
            }
        }
    } else {
        switch (o->type) {
        case 1:
            d->heading[15] = 0;
            d->period = 6;
            break;
        case 2:
            d->heading[15] = 0;
            d->period = 16;
            break;
        case 3:
            d->heading[15] = Rand_Next(1);
            d->period = 12;
            break;
        case 4:
        case 5:
        case 6:
            o->timer = D_007DCDD4[o->type];
            d->heading[15] = Rand_Next(1);
            d->period = 6;
            break;
        case 7:
        case 8:
        case 9:
            o->timer = D_007DCDD9[o->type];
            if (Rand_Next(1) % 2 == 1) {
                sign = -1;
            } else {
                sign = 1;
            }
            d->spin = (u16)(s32)(0.5f + 65536.0f * (sign * ((20.0f + 10.0f * Lure_RandUnit()) / 8.0f)) / 360.0f);
            d->heading[15] = Rand_Next(1);
            d->period = 8;
            break;
        }
        o->slot = DrawSlot_Alloc();
        if (o->slot != -1) {
            o->node = DrawSlot_Get(o->slot);
            o->node->owner = o;
            o->node->draw = VObj_Draw;
        } else {
            StageObj_Free(o);
        }
    }
}

u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
void VObj_ResetParts(VObj *o)
{
    VObjData *d = (VObjData *)o->buf;
    u8 i;

    d->unk154 = 12;
    d->unk155 = 0;
    d->finished = 0;
    d->delayLeft = d->delay;
    for (i = 0; i < 15; i++) {
        d->flag[i] = 0;
        d->ang[i] = 0xA000;
        d->heading[i] = Math_Heading(d->pos.x, d->pos.z, 0.0f, 0.0f) + 0x8000;
        d->unk120[i] = 0;
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gallery_BeforeShooting(void);
extern f32 gVObjHeights[];

/* Run (state 1). */
void VObj_Run(VObj *o)
{
    VObjData *d = (VObjData *)o->buf;
    u8 *kinds = d->kinds;
    Vec3f dir;
    u8 i;

    switch (o->type) {
    case 0:
        if (Gallery_BeforeShooting() == 0) {
            o->state = 2;
            VObj_End(o);
            break;
        }
        if (d->finished == 1) {
            VObj_ResetParts(o);
            break;
        }
        for (i = 0; i < 15; i++) {
            if (d->parts[i] != NULL) {
                if (kinds[i] > 0) {
                    if (d->flag[i] == 1) {
                        if (++d->unk120[i] < 6) {
                            d->ang[i] -= 0xCCD;
                        } else {
                            if (gVObjKinds[kinds[i]].unk7 == 1) {
                                d->finished = 1;
                                d->unk154 = 12;
                                o->var0 = 0;
                                d->unk155 = 1;
                            }
                            kinds[i] = 0;
                            d->unk111[i] = 0;
                        }
                    } else if (o->var0 <= 0) {
                        if (d->unk155 == 1) {
                            d->ang[i] -= d->spin;
                        } else {
                            d->ang[i] += d->spin;
                        }
                    }
                }
                Vec_Copy(d->parts[i]->pos, &d->spots[d->spotIdx[i]].x);
                d->parts[i]->pos[1] += gVObjHeights[gVObjKinds[kinds[i]].unk5] / 2.0f;
                OT_AddByDepth(gDrawOt32, d->parts[i], 0x40, 0);
            }
        }
        switch (o->var0) {
        case 0:
            d->finished = 0;
            if ((*(s8 *)&d->unk154)-- <= 0) {
                if (d->unk155 == 0) {
                    o->var0 = 1;
                } else {
                    *((s8 *)d + 0x1C0) = 10;
                    o->var0 = 2;
                }
            }
            break;
        case 1:
            if ((*(s8 *)&d->delayLeft)-- <= 0) {
                d->unk155 = 1;
                d->unk154 = 12;
                o->var0 = 0;
            }
            break;
        case 2:
            if (--*((s8 *)d + 0x1C0) < 0) {
                d->finished = 1;
                o->var0 = 0;
            }
            break;
        }
        break;
    case 1:
    case 2:
    case 3:
        if (++o->timer > d->period) {
            o->timer = 0;
            o->state = 2;
            break;
        }
        dir.x = ((f32 *)&gCamDir)[0];
        dir.y = ((f32 *)&gCamDir)[1];
        dir.z = ((f32 *)&gCamDir)[2];
        Vec_Normalize(&dir.x);
        o->pos[0] += 5.0f * dir.x;
        o->pos[1] += 5.0f * dir.y;
        o->pos[2] += 5.0f * dir.z;
        Vec_Copy(o->node->pos, &o->pos[0]);
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        break;
    case 4:
    case 5:
    case 6:
        if (++o->timer >= 0) {
            if (o->timer > 6) {
                o->state = 2;
            } else {
                Vec_Copy(o->node->pos, &o->pos[0]);
                OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
            }
        }
        break;
    case 7:
    case 8:
    case 9:
        if (++o->timer >= 0) {
            if (o->timer > 8) {
                o->state = 2;
            } else {
                d->heading[15] += d->spin;
                Vec_Copy(o->node->pos, &o->pos[0]);
                OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
            }
        }
        break;
    }
}

void VObj_End(VObj *o)
{
    VObjData *d = (VObjData *)o->buf;
    u8 i;

    switch (o->type) {
    case 0:
        o->state = 3;
        for (i = 0; i < 15; i++) {
            if (d->parts[i] != NULL) {
                DrawSlot_Free(d->slot[i]);
            }
        }
        break;
    default:
        o->state = 3;
        if (o->node != NULL) {
            DrawSlot_Free(o->slot);
        }
        break;
    }
}

void VObj_Free(VObj *o)
{
    StageObj_Free(o);
}

/* unk_6B7830's .data (0x7DCAC0-0x7DCBB0) */
f32 gVObjHeights[4] = {
#include "lobby/007DCAC0.inc"
};
VKind gVObjKinds[22] = {
#include "lobby/007DCAD0.inc"
};
Vec3f gVObjRowBases[4] = {
#include "lobby/007DCB80.inc"
};
