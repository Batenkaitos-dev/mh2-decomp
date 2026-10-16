#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"

/* prototypes */
u16 Rand_Next(s32 a);
void StageObj_Free(StageObj *o);
void DrawSlot_Free(s32 id);
void Effect0D_Setup(StageObj *o);
void Effect0D_Run(StageObj *o);
void Effect0D_End(StageObj *o);
void Effect0D_Delete(StageObj *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
StageObj *StageObj_Alloc(s32 nblk);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Copy(f32 *dst, f32 *src);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Vec_Scale(f32 *out, f32 *v, f32 s);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
extern f32 gCamMatrix[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Effect0D_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gs_SetState(u8 reg, u32 v);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_Concat3x3(f32 *m, f32 *a);
s16 Effect_MapTexId(s16 id);

/* The kind 0xD effect objects (Effect0D_*). Built at optimization level 4:
 * Effect0D_Run matches only there (every other function at both levels). */
#pragma optimization_level 4

/* An update of effect objects, by state: set up, run, end, free. */
void Effect0D_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        Effect0D_Setup(o);
        break;
    case 1:
        Effect0D_Run(o);
        break;
    case 2:
        Effect0D_End(o);
        break;
    case 3:
        Effect0D_Delete(o);
        break;
    }
}

/* A part of a kind 0xD effect object (StageObj.buf, 0x30 bytes each). */
typedef struct EffectPartD {
    s16 unk00;                  /* 0x00 */
    s16 unk02;                  /* 0x02 */
    f32 pos[3];                 /* 0x04 */
    f32 unk10[3];               /* 0x10: a key curve (Effect0D_Run) */
    f32 scale;                  /* 0x1C */
    f32 unk20;                  /* 0x20: a key curve (Effect0D_Run) */
    u16 unk24;                  /* 0x24: a heading */
    u16 unk26;                  /* 0x26 */
    s16 unk28;                  /* 0x28 */
    s16 slot;                   /* 0x2A: its draw slot */
    DrawSlot *draw;             /* 0x2C: NULL when none */
} EffectPartD;
void Vec_RotateY(f32 *v, f32 angle);
void Effect0D_PlaySound(StageObj *o);
void Effect0D_DrawPart(DrawSlot *s);
extern s16 gEff0DPartCounts[];
extern s16 gEff0DType14Starts[];
extern s16 gEff0DType19Starts[];
extern s16 gEff0DType35Starts[];
extern s16 gEff0DType36Starts[];
/* The set up of a kind 0xD effect object: its parts (gEff0DPartCounts: the count of each type) with their draw slots (Effect0D_DrawPart). */
void Effect0D_Setup(StageObj *o)
{
    s16 n;
    s16 i;
    EffectPartD *part = (EffectPartD *)o->buf;
    f32 v[3];

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    n = gEff0DPartCounts[o->type];
    switch (o->type) {
    case 9:
        if (o->var0 == 1) {
            v[0] = 10.0f;
            v[1] = 0.0f;
            v[2] = -100.0f;
        } else {
            v[0] = -10.0f;
            v[1] = 0.0f;
            v[2] = -100.0f;
        }
        Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        break;
    case 19:
    case 20:
    case 21:
    case 26:
    case 29:
    case 40:
        o->angle = Rand_Next(1);
        break;
    }
    Effect0D_PlaySound(o);
    for (i = 0; i < n; part++, i++) {
        part->scale = 1.0f;
        part->unk02 = 0;
        switch (o->type) {
        case 0:
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 3:
        case 18:
        case 30:
        case 32:
        case 33:
            part->scale = o->scale;
            part->unk26 = (s16)(0.5f + 65536.0f * (0.09f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 5:
        case 10:
            part->scale = o->scale * (1.0f + 0.0005f * (Rand_Next(1) & 0x3FF));
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 11:
        case 12:
        case 15:
        case 17:
        case 23:
        case 24:
        case 25:
        case 38:
        case 39:
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 13:
            part->unk02 = -i * 2;
            part->scale = o->scale * (1.0f + 0.2f * (0.001f * (Rand_Next(1) & 0x3FF)));
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 14:
            part->scale = o->scale;
            part->unk02 = gEff0DType14Starts[i];
            switch (i) {
            case 0:
                part->unk00 = 0;
                break;
            case 1:
            case 2:
                part->unk00 = 1;
                break;
            }
            part->unk26 = 0;
            part->unk28 = 0;
            break;
        case 16:
        case 31:
        case 37:
            part->unk02 = -1;
            part->scale = o->scale;
            part->unk26 = 0;
            part->unk28 = 0;
            break;
        case 19:
            part->unk02 = gEff0DType19Starts[i] - 6;
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        case 20:
        case 27:
            part->unk02 = 0;
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            break;
        case 21:
        case 26:
        case 29:
            part->scale = o->scale;
            if (i == 0) {
                part->scale *= 2.0f;
                part->unk02 = 0;
            } else if (i == 5) {
                part->unk02 = -2;
            } else {
                part->unk02 = 0;
            }
            part->unk26 = 0;
            part->unk28 = 0;
            break;
        case 22:
        case 28:
            part->unk02 = 0;
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            part->unk28 = (Rand_Next(1) & 0xFF) - 0x80;
            break;
        case 34:
            part->unk02 = -i * 3;
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            Vec_Copy(part->pos, o->pos);
            break;
        case 35:
            part->unk02 = gEff0DType35Starts[i];
            part->scale = o->scale;
            part->unk00 = i;
            Vec_Copy(part->pos, o->pos);
            part->pos[1] += 30.0f;
            switch (part->unk00) {
            case 0:
            case 5:
                part->unk26 = 0;
                part->unk28 = 0;
                break;
            case 1:
                part->unk26 = Rand_Next(1);
                part->unk28 = 0x1111;
                break;
            case 2:
            case 3:
            case 4:
            default:
                part->unk26 = Rand_Next(1);
                part->unk28 = 0xCCD;
                break;
            }
            break;
        case 36:
            part->unk02 = gEff0DType36Starts[i];
            part->scale = o->scale;
            part->unk00 = i & 1;
            Vec_Copy(part->pos, o->pos);
            part->unk26 = Rand_Next(1);
            if (part->unk00 == 0) {
                part->unk28 = 0x666;
            } else {
                part->unk28 = -0x665;
            }
            break;
        case 40:
            part->unk02 = gEff0DType19Starts[i];
            part->scale = o->scale;
            part->unk26 = Rand_Next(1);
            part->unk28 = (s16)(0.5f + 65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200)) / 360.0f);
            break;
        }
        part->slot = DrawSlot_Alloc();
        if (part->slot != -1) {
            part->draw = DrawSlot_Get(part->slot);
            part->draw->owner = o;
            part->draw->part = i;
            part->draw->draw = Effect0D_DrawPart;
        } else {
            part->draw = NULL;
        }
    }
}

void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
u16 Mtx_Heading(f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gCamMatrix[];
extern void *gEffect0DKeys[];
extern u16 gEff0DPartLens[];
extern s16 gEff0DKeyStarts[];
extern s16 gEff0DKeyStrides[];
extern u16 gEff0DType14Lens[];
extern u16 gEff0DType21Lens[];
extern u16 gEff0DType35Lens[];
/* The run of a kind 0xD effect object: each part ages and follows its key curves (gEffect0DKeys), then goes into the ordering table.
 * Level 4 (the file is at 3). */
void Effect0D_Run(StageObj *o)
{
    s16 i;
    s16 n = gEff0DPartCounts[o->type];
    u16 dur = gEff0DPartLens[o->type];
    s16 key = gEff0DKeyStarts[o->type];
    s16 stride = gEff0DKeyStrides[o->type];
    s16 step;
    EffectPartD *part = (EffectPartD *)o->buf;
    u16 len;
    void *keys;
    f32 m[16];
    f32 v[3];

    switch (o->type) {
    case 6:
        step = 0x2AAA;
        break;
    case 8:
        step = 0x2AAA;
        break;
    case 7:
    case 11:
    case 12:
    case 15:
    case 23:
    case 24:
    case 25:
    case 38:
    case 39:
        step = 0x3333;
        break;
    case 19:
        step = 0x2000;
        break;
    case 40:
        step = 0x2000;
        break;
    case 21:
    case 26:
    case 29:
        o->angle += 0xB6;
        break;
    }
    if (++o->timer > dur) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 6:
        case 8:
        case 7:
        case 11:
        case 12:
        case 15:
        case 17:
        case 19:
        case 23:
        case 24:
        case 25:
        case 38:
        case 39:
        case 40:
            key = gEff0DKeyStarts[o->type];
            len = dur;
            break;
        case 13:
            key = gEff0DKeyStarts[o->type];
            len = 12;
            break;
        case 14:
            len = gEff0DType14Lens[i];
            break;
        case 21:
        case 26:
        case 29:
            len = gEff0DType21Lens[i];
            break;
        case 34:
            key = gEff0DKeyStarts[o->type];
            len = 14;
            break;
        case 35:
            len = gEff0DType35Lens[i];
            break;
        case 36:
            key = gEff0DKeyStarts[o->type];
            len = 10;
            break;
        default:
            len = dur;
            break;
        }
        if (++part->unk02 <= 0) {
            part++;
            key += stride;
            continue;
        }
        if (part->unk02 == 1) {
            if (o->type == 13) {
                if (((Player *)o->owner)->active != 0) {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 2)));
                    Mtx_GetTrans(part->pos, m);
                    part->pos[1] = ((Player *)o->owner)->floorY;
                    part->unk24 = Mtx_Heading(m) + 0x6000;
                } else {
                    part->unk00 = 0xFF;
                    part++;
                    key += stride;
                    continue;
                }
            }
        } else if (part->unk02 > len) {
            part++;
            key += stride;
            continue;
        }
        if (part->unk00 == 0xFF) {
            return;
        }
        Key_LerpVec3(gEffect0DKeys[key], part->unk10, part->unk02);
        switch (o->type) {
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 22:
        case 23:
        case 24:
        case 25:
        case 28:
        case 30:
        case 31:
        case 32:
        case 33:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
            Key_LerpVec3(gEffect0DKeys[(s16)(key + 1)], part->pos, part->unk02);
            part->pos[0] *= o->scale;
            part->pos[1] *= o->scale;
            part->pos[2] *= o->scale;
            keys = gEffect0DKeys[(s16)(key + 2)];
            key += 3;
            break;
        case 13:
            Key_LerpVec3(gEffect0DKeys[(s16)(key + 1)], v, part->unk02);
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            keys = gEffect0DKeys[(s16)(key + 2)];
            key += 3;
            break;
        default:
            keys = gEffect0DKeys[(s16)(key + 1)];
            key += 2;
            break;
        }
        Key_LerpFloat(keys, &part->unk20, part->unk02);
        if (part->draw != NULL) {
            switch (o->type) {
            case 3:
            case 18:
            case 30:
            case 32:
            case 33:
                v[0] = part->pos[0];
                v[1] = part->pos[1];
                v[2] = 20.0f + part->pos[2];
                Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
                part->draw->pos[0] = v[0] + o->pos[0];
                part->draw->pos[1] = v[1] + o->pos[1];
                part->draw->pos[2] = v[2] + o->pos[2];
                break;
            case 0:
            case 4:
            case 5:
            case 9:
            case 10:
            case 14:
            case 16:
            case 17:
            case 22:
            case 28:
            case 31:
            case 37:
                part->unk26 += (u16)part->unk28;
                Vec_Copy(v, part->pos);
                Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
                part->draw->pos[0] = v[0] + o->pos[0];
                part->draw->pos[1] = v[1] + o->pos[1];
                part->draw->pos[2] = v[2] + o->pos[2];
                break;
            case 6:
            case 7:
            case 8:
            case 11:
            case 12:
            case 15:
            case 19:
            case 23:
            case 24:
            case 25:
            case 38:
            case 39:
            case 40:
                part->unk26 += (u16)part->unk28;
                Vec_Copy(v, part->pos);
                Vec_RotateY(v, 6.2831855f * (o->angle + i * step) / 65536.0f);
                part->draw->pos[0] = v[0] + o->pos[0];
                part->draw->pos[1] = v[1] + o->pos[1];
                part->draw->pos[2] = v[2] + o->pos[2];
                break;
            case 13:
                part->unk26 += (u16)part->unk28;
                Vec_RotateY(v, 6.2831855f * part->unk24 / 65536.0f);
                part->draw->pos[0] = v[0] + part->pos[0];
                part->draw->pos[1] = v[1] + part->pos[1];
                part->draw->pos[2] = v[2] + part->pos[2];
                break;
            case 20:
            case 27:
                Vec_Copy(part->draw->pos, o->pos);
                break;
            case 21:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 30.0f;
                if (o->owner != NULL && ((Player *)o->owner)->active != 0 && ((Player *)o->owner)->weaponType == 8) {
                    v[2] += 60.0f;
                }
                Vec_RotateSelf(v, gCamMatrix);
                part->draw->pos[0] = o->pos[0] + v[0];
                part->draw->pos[1] = o->pos[1] + v[1];
                part->draw->pos[2] = o->pos[2] + v[2];
                break;
            case 26:
            case 29:
                part->draw->pos[0] = o->pos[0];
                part->draw->pos[1] = o->pos[1];
                part->draw->pos[2] = o->pos[2];
                break;
            case 34:
                part->pos[1] += 2.0f;
                part->draw->pos[0] = part->pos[0];
                part->draw->pos[1] = part->pos[1];
                part->draw->pos[2] = part->pos[2];
                break;
            case 35:
                part->unk26 += (u16)part->unk28;
                switch (part->unk00) {
                case 0:
                    if (part->unk02 > 2) {
                        part->pos[1] += 33.3f;
                    }
                    break;
                case 2:
                    if (part->unk02 > 2) {
                        part->pos[1] += 25.0f;
                    }
                    break;
                case 3:
                    if (part->unk02 > 2) {
                        part->pos[1] += 18.7f;
                    }
                    break;
                case 4:
                    if (part->unk02 > 2) {
                        part->pos[1] += 12.5f;
                    }
                    break;
                }
                Vec_Copy(part->draw->pos, part->pos);
                break;
            case 36:
                part->unk26 += (u16)part->unk28;
                Vec_Copy(v, part->pos);
                if (part->unk00 != 0) {
                    v[0] = -v[0];
                }
                Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
                part->draw->pos[0] = v[0] + o->pos[0];
                part->draw->pos[1] = v[1] + o->pos[1];
                part->draw->pos[2] = v[2] + o->pos[2];
                break;
            }
            OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
        }
        part++;
    }
}

extern s16 gEff0DPartCounts[];
/* Frees a kind 0xD effect object's draw slots (gEff0DPartCounts: the parts of each type). */
void Effect0D_End(StageObj *o)
{
    s16 n;
    s16 i;
    EffectPartD *part = (EffectPartD *)o->buf;

    o->state++;
    n = gEff0DPartCounts[o->type];
    for (i = 0; i < n; i++, part++) {
        if (part->draw != NULL) {
            DrawSlot_Free(part->slot);
        }
    }
}

void Effect0D_Delete(StageObj *o)
{
    StageObj_Free(o);
}

void Effect_GetColor(s32 arg0, u8 *arg1, u8 *arg2, u8 *arg3, u8 *arg4);
s32 Model_DrawFade(void *mdl, f32 *a1, u16 flags, void *a3, f32 alpha);
void Mtx_TurnZ(f32 *m, f32 a);
void Gfx_SetState63(s32 a);
extern f32 gCamYawMtx[];
extern u8 gEff0DPlayerColors[][3];
/* Draws a part of a kind 0xD effect object (its draw slot): a model of set 0 by the object type, tinted by the area light. */
void Effect0D_DrawPart(DrawSlot *s)
{
    StageObj *o = s->owner;
    EffectPartD *part = &((EffectPartD *)o->buf)[s->part];
    void *base;
    u16 flags = 0;
    f32 m[16];
    f32 rot[3];
    f32 scale[3];
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    s16 id;
    f32 bright;
    ModelMatSet *mdl;
    Model *set = gCommonModels[0];

    if (set != NULL && set->active) {
        base = set->mats;
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        bright = a / 255.0f;
        switch (o->type) {
        case 0:
            Gs_SetState(0x6C, 0);
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            id = 0x4A;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 3:
        case 4:
        case 6:
        case 7:
        case 9:
        case 11:
        case 12:
        case 13:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 23:
        case 24:
        case 25:
        case 27:
        case 31:
        case 32:
        case 33:
        case 34:
        case 37:
        case 38:
        case 39:
        case 40:
            Gs_SetState(0x6C, 0);
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            id = 0x4A;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 5:
        case 10:
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            id = 0x4A;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 8:
            scale[0] = 0.8f * part->unk10[0];
            scale[1] = 0.8f * part->unk10[1];
            scale[2] = 0.8f * part->unk10[2];
            id = 0x4A;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 14:
            switch (part->unk00) {
            case 0:
                id = 0x4A;
                r = 0xFF;
                g = 0xFF;
                b = 0xFF;
                bright = 1.0f;
                break;
            case 1:
                id = 0x23;
                r = 0x33;
                g = 0x33;
                b = 0x33;
                bright = 1.0f;
                break;
            }
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 21:
        case 26:
        case 29:
            if (s->part == 0) {
                id = 0x41;
            } else {
                id = 0x40;
            }
            flags |= 2;
            if (o->type == 0x15) {
                r = 0xBF;
                b = 0xFF;
                g = 0xBF;
            } else if (o->type == 0x1D) {
                r = 0xFF;
                g = 0x7F;
                b = 0x7F;
            } else if (gScene.online == 0) {
                r = gEff0DPlayerColors[o->ownerIndex][0];
                g = gEff0DPlayerColors[o->ownerIndex][1];
                b = gEff0DPlayerColors[o->ownerIndex][2];
            } else {
                r = 0xFF;
                g = 0xFF;
                b = 0xFF;
            }
            bright = 1.0f;
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            rot[2] = 6.2831855f * (part->unk26 + o->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 22:
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            r = 0xFF;
            g = 0xFF;
            b = 0xFF;
            bright = 1.0f;
            id = 0x48;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 28:
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            r = 0xD8;
            g = 0xCE;
            b = 0xB9;
            bright = 1.0f;
            id = 0x48;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 30:
            scale[0] = part->scale * part->unk10[0];
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            r = 0xFF;
            g = 0xFF;
            b = 0xFF;
            bright = 1.0f;
            id = 0x4A;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 35:
            Gs_SetState(0x6C, 0);
            flags |= 2;
            scale[0] = part->scale * part->unk10[0];
            bright = 1.0f;
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            switch (part->unk00) {
            case 0:
                r = 0xFF;
                id = 0x4A;
                g = 0x33;
                b = 0x33;
                Mtx_FromScaleRotTrans(scale, rot, s->pos, 0, m);
                Mtx_Concat3x3(m, gCamYawMtx);
                break;
            case 1:
                r = 0xFF;
                g = 0xFF;
                b = 0xFF;
                id = 0x51;
                rot[2] = 6.2831855f * part->unk26 / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            case 2:
            case 3:
            case 4:
            default:
                id = 0x26;
                r = 0xFF;
                g = 0x33;
                b = 0x33;
                switch (part->unk00) {
                case 2:
                    rot[0] = -0.7853982f;
                    rot[1] = -0.7853982f;
                    break;
                case 3:
                    rot[0] = 0.7853982f;
                    rot[1] = -0.7853982f;
                    break;
                case 4:
                    rot[0] = 2.3561945f;
                    rot[1] = 0.0f;
                    break;
                default:
                    rot[0] = 0.0f;
                    rot[1] = 0.0f;
                    break;
                }
                rot[1] += 6.2831855f * o->angle / 65536.0f;
                rot[2] = 6.2831855f * part->unk26 / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, s->pos, 4, m);
                Mtx_TurnZ(m, rot[2]);
                break;
            case 5:
                id = 0x3B;
                r = 0xFF;
                g = 0xAF;
                b = 0x6F;
                rot[2] = 6.2831855f * part->unk26 / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, s->pos, 3, m);
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            }
            break;
        case 36:
            Gs_SetState(0x6C, 0);
            flags |= 2;
            scale[0] = part->scale * part->unk10[0];
            bright = 1.0f;
            scale[1] = part->scale * part->unk10[1];
            scale[2] = part->scale * part->unk10[2];
            r = 0xFF;
            g = 0x33;
            b = 0x33;
            rot[0] = -0.5235988f;
            id = 0x26;
            if (part->unk00 == 0) {
                rot[1] = -1.308997f;
            } else {
                rot[1] = 1.308997f;
            }
            rot[1] += 6.2831855f * o->angle / 65536.0f;
            rot[2] = 6.2831855f * part->unk26 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, s->pos, 4, m);
            Mtx_TurnZ(m, rot[2]);
            break;
        }
        id = Effect_MapTexId(id);
        if (id != -1) {
            mdl = &set->matSets[id];
            if ((u32)(o->type - 1) <= 1 || o->type == 0x10) {
                Model_DrawFade(mdl, m, flags, base, part->unk20);
            } else {
                Effect_DrawModel(mdl, m, ((u8)(u32)(255.0f * bright * part->unk20) << 24) | (r << 16) | (g << 8) | b, flags, base);
            }
        }
        Gfx_SetState63(1);
        Gs_SetState(0x6C, 1);
    }
}

/* The sound of a kind 0xD effect object, by type (0x1A when its owner, or the player in +0x1E, is the local player: gScene.localPlayer is SceneState.localPlayer; 0x23 for long sword users). */
void Effect0D_PlaySound(StageObj *o)
{
    Player *p;

    switch (o->type) {
    case 26:
        if (((Player *)o->owner)->index == gScene.localPlayer || o->ownerIndex == gScene.localPlayer) {
            Sound_PlaySeAtPosChecked(0, 0x1A, 0, o->pos, 1, 0);
        }
        break;
    case 35:
        p = o->owner;
        if (p->weaponType == 7) {
            Player_PlaySe(p, 0xC, 0, p->pos, 1, 0);
        }
        break;
    }
}

/* unit's own declaration: with the definition's u8 (f32 *pos, f32 *y) (sub_main/unk_328CD0.c), Effect0D_GroundHeight: 17 of 73 words differ */
s32 AreaGrid2_GetFloorHeight(f32 *pos, f32 *h);
f32 Area_GroundOffset(u16 area);
extern s16 gEff0DOnGround[];
extern s16 gAreaKinds[];
/* The height for an effect of the given type at pos (on the ground below it, 5 up); 0 when the type has none (gEff0DOnGround) or there is no ground. */
s32 Effect0D_GroundHeight(Player *p, f32 *pos, s32 type)
{
    f32 h;
    s16 *k = &gEff0DOnGround[(s16)type];
    s16 area;

    if (*k == 0) {
        return 0;
    }
    if (p != NULL) {
        pos[1] = p->floorY;
    }
    if (AreaGrid2_GetFloorHeight(pos, &h) != 0 && !(h < pos[1] - 18.0f)) {
        pos[1] = 5.0f + h;
        return *k;
    }
    area = gAreaKinds[gScene.area];
    if (area == 10 || area == 14) {
        pos[1] += 5.0f;
        pos[1] += Area_GroundOffset(gScene.area);
        return *k;
    }
    return 0;
}

u16 Mtx_Heading(f32 *m);
/* Makes o a kind 0xD effect object of the given type for the player (Effect0D_Update updates it). */
void Effect0D_Init(Player *p, s16 arg, s32 type, StageObj *o)
{
    f32 m[4][4];

    o->kind = 0xD;
    o->update = Effect0D_Update;
    o->type = type;
    o->angle = p->rot[1];
    switch (o->type) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 18:
    case 20:
        if (arg == 8) {
            o->var0 = 0;
        } else {
            o->var0 = 1;
        }
        break;
    case 10:
        o->scale = 0.7f;
        break;
    case 13:
        o->var0 = arg;
        break;
    case 22:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        o->angle = Mtx_Heading(m[0]) + 0x4000;
        break;
    case 26:
        o->ownerIndex = ((u8 *)p)[0x861];
        break;
    }
    o->owner = p;
}

f32 AreaGrid2_GetFloorY(f32 *pos);
void Vec_RotateY(f32 *v, f32 angle);
void StageObj_K11_CreateAtScaled(f32 *pos, u16 ang, s32 n, f32 scale); /* unit's own declaration: the definition (game) is void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale); the calls here match only with this one */
void Effect0D_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale);
/* Where on the player to put an effect of the given type (bone arg), then its ground height (Effect0D_GroundHeight). */
s32 Effect0D_PlayerPos(f32 *pos, Player *p, s16 arg, s32 type)
{
    f32 m[4][4];
    f32 v[3];
    f32 f;
    s32 res;

    switch ((s16)type) {
    case 0:
    case 3:
    case 4:
    case 6:
    case 7:
    case 9:
    case 18:
    case 20:
        Em_GetBonePos(p, arg, pos);
        break;
    case 5:
        Em_GetBonePos(p, arg, pos);
        v[0] = 0.0f;
        v[1] = -19.0f;
        v[2] = -13.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        Vec_Add(pos, pos, v);
        break;
    case 8:
    case 39:
        pos[0] = p->pos[0];
        pos[1] = p->floorY;
        pos[2] = p->pos[2];
        if (p->motionId[0] == 0x13) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 50.0f;
            Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
        }
        break;
    case 10:
        Em_GetBonePos(p, arg, pos);
        break;
    case 11:
        Em_GetBonePos(p, arg, pos);
        pos[1] = p->floorY;
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 20.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        break;
    case 12:
    case 19:
    case 40:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = 44.0f;
        v[1] = 0.0f;
        v[2] = 123.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        f = AreaGrid2_GetFloorY(pos);
        if (pos[1] < 50.0f + f) {
            pos[1] = f;
        }
        break;
    case 13:
        Em_GetBonePos(p, arg, pos);
        pos[1] = p->floorY;
        break;
    case 14:
    case 15:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = 44.0f;
        v[1] = 0.0f;
        v[2] = 123.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 16:
        pos[0] = p->pos[0];
        pos[1] = p->floorY;
        pos[2] = p->pos[2];
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = -50.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        break;
    case 17:
        Em_GetBonePos(p, arg, pos);
        pos[1] = p->floorY;
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = -25.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        Vec_Add(pos, pos, v);
        break;
    case 21:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        switch (p->weaponType) {
        case 0:
        case 7:
            v[0] = 10.0f;
            v[1] = -2.0f;
            v[2] = 190.0f;
            break;
        case 4:
            v[0] = -4.9f;
            v[1] = -0.7f;
            v[2] = 90.0f;
            break;
        case 3:
        case 9:
            v[0] = -7.8f;
            v[1] = -10.0f;
            v[2] = 220.0f;
            break;
        case 2:
            v[0] = -5.0f;
            v[1] = -20.0f;
            v[2] = 155.0f;
            break;
        case 8:
            v[0] = 0.0f;
            v[1] = -50.0f;
            v[2] = 180.0f;
            break;
        case 6:
            v[0] = -4.9f;
            v[1] = -0.7f;
            v[2] = 90.0f;
            break;
        default:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            break;
        }
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        break;
    case 22:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = 0.0f;
        v[1] = -2.0f;
        v[2] = 16.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        break;
    case 23:
    case 35:
        Em_GetBonePos(p, arg, pos);
        break;
    case 24:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = -5.0f;
        v[1] = 0.0f;
        v[2] = 50.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = p->floorY;
        pos[2] = m[3][2] + v[2];
        break;
    case 25:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 125.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        pos[0] = p->pos[0] + v[0];
        pos[1] = p->pos[1] + v[1];
        pos[2] = p->pos[2] + v[2];
        break;
    case 26:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = 5.0f;
        v[1] = -15.0f;
        v[2] = 10.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        break;
    case 31:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_GetBone(p, arg)));
        v[0] = 44.0f;
        v[1] = 0.0f;
        v[2] = 123.0f;
        Vec_RotateSelf(v, m[0]);
        pos[0] = m[3][0] + v[0];
        pos[1] = m[3][1] + v[1];
        pos[2] = m[3][2] + v[2];
        f = AreaGrid2_GetFloorY(pos);
        if (pos[1] < 50.0f + f) {
            pos[1] = f;
        }
        break;
    case 33:
        pos[0] = 13446.23f;
        pos[1] = 70.0f;
        pos[2] = 14372.39f;
        break;
    case 36:
        Em_GetBonePos(p, arg, pos);
        pos[1] = 90.0f + p->floorY;
        break;
    case 37:
    case 38:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 130.0f;
        Vec_RotateY(v, 6.2831855f * p->rot[1] / 65536.0f);
        pos[0] = p->pos[0] + v[0];
        pos[1] = p->floorY;
        pos[2] = p->pos[2] + v[2];
        break;
    }
    res = (s16)Effect0D_GroundHeight(p, pos, type);
    if ((s16)type == 0x13 && (s16)res == 0) {
        v[0] = pos[0];
        v[1] = 20.0f + pos[1];
        v[2] = pos[2];
        StageObj_K11_CreateAtScaled(v, p->rot[1], 10, 0.4f);
        Effect0D_SpawnForPlayerScaled(p, 0xE, 0xE, 1.0f);
    }
    return res;
}

s32 Effect0D_PlayerPos(f32 *pos, Player *p, s16 arg, s32 type);
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 k, f32 scale);
void Effect0D_Init(Player *p, s16 arg, s32 type, StageObj *o);
void Effect0D_SpawnForPlayer(Player *p, s16 arg, s32 type)
{
    f32 pos[3];
    StageObj *o;

    if ((u8)Obj_InCurrentArea(p) != 0 && Effect_SpawnGround(p, pos, Effect0D_PlayerPos(pos, p, arg, type), 1.0f) == 0) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(o->pos, pos);
            o->scale = 1.0f;
            Effect0D_Init(p, arg, type, o);
        }
    }
}

s32 Effect0D_PlayerPos(f32 *pos, Player *p, s16 arg, s32 type);
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 k, f32 scale);
void Effect0D_Init(Player *p, s16 arg, s32 type, StageObj *o);
void Effect0D_SpawnForPlayerScaled(Player *p, s16 arg, s32 type, f32 scale)
{
    f32 pos[3];
    StageObj *o;

    if ((u8)Obj_InCurrentArea(p) != 0 && Effect_SpawnGround(p, pos, Effect0D_PlayerPos(pos, p, arg, type), 1.0f) == 0) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(o->pos, pos);
            Effect0D_Init(p, arg, type, o);
            o->scale = scale;
        }
    }
}

/* unit's own declaration: with the definition's EmBone *(void *obj, s16 bone) (sub_main/unk_3DDBA0.c), Effect0D_EmPos: 157 of 534 words differ, Effect0D_InitForEm: 57 of 479 words differ */
EmBone *Em_Bone(void *obj, s32 a);
u8 Em_TypeUsesK14(u8 type); /* unit's own declaration: the definition (game) is s32 Em_TypeUsesK14(u8 type); the calls here match only with this one */
void Eff01_CreateOnObj(StageObj *s, s8 kind);
void Eff05_CreateOnObj(StageObj *s, s8 kind);
/* The monster twin of Effect0D_Init. */
void Effect0D_InitForEm(Em *em, s32 a, s32 type, StageObj *o)
{
    f32 m[4][4];

    o->kind = 0xD;
    o->update = Effect0D_Update;
    o->type = type;
    o->angle = em->rotY;
    o->owner = em;
    switch (o->type) {
    case 0:
        switch (em->type) {
        case 0xC:
        case 0x46:
        case 0x19:
            o->scale = 4.0f * em->scaleX;
            break;
        case 0x12:
            o->scale = 0.5f * em->scaleX;
            break;
        case 0x10:
        case 0x23:
        case 0xD:
        case 0x1B:
        case 0x1C:
        case 0x1E:
        case 0x1F:
            o->scale = 2.0f * em->scaleX;
            break;
        }
        break;
    case 3:
        switch (em->type) {
        case 4:
        case 5:
        case 0x44:
        case 0x20:
            break;
        case 9:
        case 0x17:
            o->scale = 2.0f * em->scaleX;
            break;
        case 0xC:
        case 0x46:
        case 0x19:
        case 0xF:
        case 0x2D:
        case 0xE:
        case 0x2B:
        case 0x1A:
        case 0x2C:
        case 3:
        case 0x45:
        case 0x21:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x3D:
        case 0x3E:
            o->scale = em->scaleX;
            break;
        }
        break;
    case 6:
        switch (em->type) {
        case 1:
        case 0x2A:
        case 0x25:
        case 6:
        case 0x26:
        case 0xB:
        case 0x29:
        case 0x31:
        case 0xE:
        case 0x2B:
        case 0x1A:
        case 0x2C:
        case 0xF:
        case 0x2D:
        case 0x11:
        case 0x2F:
        case 0x14:
        case 0x27:
        case 0x15:
        case 0x2E:
        case 0x28:
            o->scale = 1.3f * em->scaleX;
            break;
        case 0xC:
        case 0x46:
        case 0x19:
            o->scale = 2.0f * em->scaleX;
            break;
        case 0x12:
            o->scale = em->scaleX;
            break;
        }
        break;
    case 7:
        switch (em->type) {
        case 4:
        case 5:
        case 0x44:
        case 0x20:
        case 0xC:
        case 0x46:
        case 0x19:
            o->scale = em->scaleX;
            break;
        case 9:
        case 0x17:
            o->scale = 0.5f * em->scaleX;
            break;
        case 0x12:
            if ((s16)a == 0) {
                o->scale = 2.0f * em->scaleX;
            } else {
                o->scale = 0.5f * em->scaleX;
            }
            break;
        case 0x11:
        case 0x2F:
        case 0x16:
        case 0xF:
        case 0x2D:
        case 0xE:
        case 0x2B:
        case 0x1A:
        case 0x2C:
        case 0xB:
        case 0x29:
        case 0x31:
        case 1:
        case 0x2A:
        case 0x25:
        case 0x14:
        case 0x27:
            o->scale = em->scaleX;
            if ((s16)a == 2) {
                if (Em_TypeUsesK14(em->type) == 0) {
                    Eff01_CreateOnObj(o, 0xE);
                } else {
                    Eff01_CreateOnObj(o, 0x2F);
                }
            }
            break;
        }
        break;
    case 19:
    case 40:
        switch (em->type) {
        case 0xE:
        case 0x2B:
        case 0x1A:
        case 0x2C:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x3D:
        case 0x3E:
            o->scale = em->scaleX;
            break;
        case 7:
        case 0x32:
            Eff05_CreateOnObj(o, 0x50);
            break;
        }
    case 20:
        switch (em->type) {
        case 0xC:
        case 0x46:
        case 0x19:
            o->scale = 5.0f * em->scaleX;
            break;
        }
        break;
    case 28:
        if ((gScene.online != 0 && em->type == 1) || (gScene.online == 0 && em->type == 10)) {
            Mtx_Copy((f32 *) m[0], (f32 *) (Em_Bone(em, a)));
            o->angle = Mtx_Heading(m[0]) + 0x4000;
        }
        break;
    case 30:
        Mtx_Copy((f32 *) m[0], (f32 *) (Em_Bone(em, a)));
        o->angle = Mtx_Heading(m[0]) - 0x4000;
        break;
    }
}

/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), Effect0D_EmPos: 420 of 534 words differ */
void Em_BonePos(void *obj, s32 part, f32 *out);
void Vec_RotateY(f32 *v, f32 angle);
/* The monster twin of Effect0D_PlayerPos: where on em to put an effect of the given type (bone a), then its ground height. */
s32 Effect0D_EmPos(f32 *pos, Em *em, s32 a, s32 b)
{
    f32 m[16];
    f32 v[3];

    switch ((s16)b) {
    case 0:
        switch (em->type) {
        case 0xC:
        case 0x46:
        case 0x19:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -10.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            break;
        case 0x12:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -20.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            break;
        default:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            break;
        }
        break;
    case 3:
        switch (em->type) {
        case 4:
        case 5:
        case 0x44:
        case 0x20:
            if ((s16)a == 0x11) {
                Em_BonePos(em, a, pos);
                Em_BonePos(em, 0xE, v);
                Vec_Add(pos, pos, v);
                Vec_Scale(pos, pos, 0.5f);
            } else {
                Em_BonePos(em, a, pos);
            }
            pos[1] = em->floorY;
            break;
        case 9:
        case 0x17:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            v[0] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            break;
        default:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            break;
        }
        break;
    case 6:
        switch (em->type) {
        default:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            break;
        case 0xC:
        case 0x46:
        case 0x19:
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            break;
        }
        break;
    case 7:
        switch (em->type) {
        default:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            break;
        case 9:
        case 0x17:
            Em_BonePos(em, a, pos);
            break;
        case 0x11:
        case 0x2F:
        case 0x16:
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, a)));
            v[0] = 100.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateSelf(v, m);
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            pos[0] += v[0];
            pos[2] += v[2];
            break;
        }
        break;
    case 19:
    case 40:
        Em_BonePos(em, a, pos);
        pos[1] = em->floorY;
        break;
    case 20:
        switch (em->type) {
        default:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            break;
        case 0xC:
        case 0x46:
        case 0x19:
            Em_BonePos(em, a, pos);
            pos[1] = em->floorY;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -10.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            break;
        }
        break;
    case 28:
        if ((gScene.online != 0 && em->type == 1) || (gScene.online == 0 && em->type == 0xA)) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, a)));
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 15.0f;
            Vec_RotateSelf(v, m);
            Mtx_GetTrans(pos, m);
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
        }
        break;
    case 30:
        Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, a)));
        v[0] = 0.0f;
        v[1] = -14.0f;
        v[2] = 45.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(pos, m);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        break;
    case 32:
        Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, a)));
        v[0] = 58.2f;
        v[1] = 0.0f;
        v[2] = -474.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(pos, m);
        pos[0] += v[0];
        pos[1] = em->floorY;
        pos[2] += v[2];
        break;
    }
    return (s16)Effect0D_GroundHeight((Player *)em, pos, b);
}

u8 Player_InCurrentArea(void *obj);
s32 Effect0D_EmPos(f32 *pos, Em *em, s32 a, s32 b);
void Effect0D_InitForEm(Em *em, s32 a, s32 b, StageObj *o);
/* The monster twin of Effect0D_SpawnForPlayer. */
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b)
{
    f32 pos[3];
    StageObj *o;

    if (Player_InCurrentArea(em) != 0 && Effect_SpawnGround(em, pos, Effect0D_EmPos(pos, em, a, b), 1.0f) == 0) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(o->pos, pos);
            o->scale = 1.0f;
            Effect0D_InitForEm(em, a, b, o);
        }
    }
}

/* The monster twin of Effect0D_SpawnForPlayerScaled, scaled by the monster's size. */
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale)
{
    f32 pos[3];
    StageObj *o;

    if (Player_InCurrentArea(em) != 0 && Effect_SpawnGround(em, pos, Effect0D_EmPos(pos, em, a, b), scale) == 0) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            Vec_Copy(o->pos, pos);
            Effect0D_InitForEm(em, a, b, o);
            o->scale = scale * em->scaleX;
        }
    }
}

void Vec_Copy(f32 *dst, f32 *src);
/* A kind 0xD effect object of the given type at pos (Effect0D_Update updates it). */
void Effect0D_SpawnAt(f32 *pos, s32 type, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0xD;
        o->update = Effect0D_Update;
        o->owner = NULL;
        o->type = type;
        Vec_Copy(o->pos, pos);
        o->scale = scale;
    }
}

void Effect0D_SpawnAt(f32 *pos, s32 type, f32 scale);
/* Effect0D_SpawnAt when o is in the current area. */
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale)
{
    if ((u8)Obj_InCurrentArea(o) != 0) {
        Effect0D_SpawnAt(pos, type, scale);
    }
}

void Effect0D_SpawnAt(f32 *pos, s32 type, f32 scale);
s32 Effect0D_GroundHeight(Player *p, f32 *pos, s32 type);
/* Effect0D_SpawnAt unless Effect_SpawnGround takes the effect. */
void Effect0D_SpawnOnGround(f32 *pos, s32 type, f32 scale)
{
    if (Effect_SpawnGround(NULL, pos, Effect0D_GroundHeight(NULL, pos, type), scale) == 0) {
        Effect0D_SpawnAt(pos, type, scale);
    }
}

/* unk_3CD9C0's .data (0x466940-0x467C40): the key curves and the tables by type */
u32 gEffect0DKeys_0[20] = {
#include "sub_main/00466940.inc"
};
u32 gEffect0DKeys_1[16] = {
#include "sub_main/00466990.inc"
};
u32 gEffect0DKeys_2[8] = {
#include "sub_main/004669D0.inc"
};
u32 gEffect0DKeys_3[24] = {
#include "sub_main/004669F0.inc"
};
u32 gEffect0DKeys_4[20] = {
#include "sub_main/00466A50.inc"
};
u32 gEffect0DKeys_5[12] = {
#include "sub_main/00466AA0.inc"
};
u32 gEffect0DKeys_6[24] = {
#include "sub_main/00466AD0.inc"
};
u32 gEffect0DKeys_7[20] = {
#include "sub_main/00466B30.inc"
};
u32 gEffect0DKeys_8[12] = {
#include "sub_main/00466B80.inc"
};
u32 gEffect0DKeys_9[24] = {
#include "sub_main/00466BB0.inc"
};
u32 gEffect0DKeys_10[20] = {
#include "sub_main/00466C10.inc"
};
u32 gEffect0DKeys_11[12] = {
#include "sub_main/00466C60.inc"
};
u32 gEffect0DKeys_12[24] = {
#include "sub_main/00466C90.inc"
};
u32 gEffect0DKeys_13[16] = {
#include "sub_main/00466CF0.inc"
};
u32 gEffect0DKeys_14[8] = {
#include "sub_main/00466D30.inc"
};
u32 gEffect0DKeys_15[20] = {
#include "sub_main/00466D50.inc"
};
u32 gEffect0DKeys_16[16] = {
#include "sub_main/00466DA0.inc"
};
u32 gEffect0DKeys_17[8] = {
#include "sub_main/00466DE0.inc"
};
u32 gEffect0DKeys_18[24] = {
#include "sub_main/00466E00.inc"
};
u32 gEffect0DKeys_19[12] = {
#include "sub_main/00466E60.inc"
};
u32 gEffect0DKeys_20[12] = {
#include "sub_main/00466E90.inc"
};
u32 gEffect0DKeys_21[16] = {
#include "sub_main/00466EC0.inc"
};
u32 gEffect0DKeys_22[16] = {
#include "sub_main/00466F00.inc"
};
u32 gEffect0DKeys_23[8] = {
#include "sub_main/00466F40.inc"
};
u32 gEffect0DKeys_24[16] = {
#include "sub_main/00466F60.inc"
};
u32 gEffect0DKeys_27[16] = {
#include "sub_main/00466FA0.inc"
};
u32 gEffect0DKeys_30[16] = {
#include "sub_main/00466FE0.inc"
};
u32 gEffect0DKeys_25[16] = {
#include "sub_main/00467020.inc"
};
u32 gEffect0DKeys_28[28] = {
#include "sub_main/00467060.inc"
};
u32 gEffect0DKeys_31[24] = {
#include "sub_main/004670D0.inc"
};
u32 gEffect0DKeys_26[8] = {
#include "sub_main/00467130.inc"
};
u32 gEffect0DKeys_29[8] = {
#include "sub_main/00467150.inc"
};
u32 gEffect0DKeys_32[8] = {
#include "sub_main/00467170.inc"
};
s16 gEff0DType14Starts[4] = {
#include "sub_main/00467190.inc"
};
u16 gEff0DType14Lens[4] = {
#include "sub_main/00467198.inc"
};
u32 gEffect0DKeys_33[16] = {
#include "sub_main/004671A0.inc"
};
u32 gEffect0DKeys_34[16] = {
#include "sub_main/004671E0.inc"
};
u32 gEffect0DKeys_35[8] = {
#include "sub_main/00467220.inc"
};
u32 gEffect0DKeys_36[24] = {
#include "sub_main/00467240.inc"
};
u32 gEffect0DKeys_37[16] = {
#include "sub_main/004672A0.inc"
};
u32 gEffect0DKeys_38[12] = {
#include "sub_main/004672E0.inc"
};
s16 gEff0DType19Starts[8] = {
#include "sub_main/00467310.inc"
};
u32 gEffect0DKeys_39[16] = {
#include "sub_main/00467320.inc"
};
u32 gEffect0DKeys_40[8] = {
#include "sub_main/00467360.inc"
};
u32 gEffect0DKeys_41[16] = {
#include "sub_main/00467380.inc"
};
u32 gEffect0DKeys_43[16] = {
#include "sub_main/004673C0.inc"
};
u32 gEffect0DKeys_45[16] = {
#include "sub_main/00467400.inc"
};
u32 gEffect0DKeys_47[20] = {
#include "sub_main/00467440.inc"
};
u32 gEffect0DKeys_49[16] = {
#include "sub_main/00467490.inc"
};
u32 gEffect0DKeys_51[16] = {
#include "sub_main/004674D0.inc"
};
u32 gEffect0DKeys_42[8] = {
#include "sub_main/00467510.inc"
};
u32 gEffect0DKeys_44[8] = {
#include "sub_main/00467530.inc"
};
u32 gEffect0DKeys_46[8] = {
#include "sub_main/00467550.inc"
};
u32 gEffect0DKeys_50[8] = {
#include "sub_main/00467570.inc"
};
u16 gEff0DType21Lens[8] = {
#include "sub_main/00467590.inc"
};
u32 gEffect0DKeys_53[20] = {
#include "sub_main/004675A0.inc"
};
u32 gEffect0DKeys_54[16] = {
#include "sub_main/004675F0.inc"
};
u32 gEffect0DKeys_55[8] = {
#include "sub_main/00467630.inc"
};
u8 gEff0DPlayerColors[10][3] = {
#include "sub_main/00467650.inc"
};
u32 gEffect0DKeys_56[20] = {
#include "sub_main/00467670.inc"
};
u32 gEffect0DKeys_57[8] = {
#include "sub_main/004676C0.inc"
};
u32 gEffect0DKeys_58[20] = {
#include "sub_main/004676E0.inc"
};
u32 gEffect0DKeys_60[16] = {
#include "sub_main/00467730.inc"
};
u32 gEffect0DKeys_62[16] = {
#include "sub_main/00467770.inc"
};
u32 gEffect0DKeys_68[12] = {
#include "sub_main/004677B0.inc"
};
u32 gEffect0DKeys_59[8] = {
#include "sub_main/004677E0.inc"
};
u32 gEffect0DKeys_61[8] = {
#include "sub_main/00467800.inc"
};
u32 gEffect0DKeys_63[8] = {
#include "sub_main/00467820.inc"
};
u32 gEffect0DKeys_69[6] = {
#include "sub_main/00467840.inc"
};
s16 gEff0DType35Starts[6] = {
#include "sub_main/00467858.inc"
};
u16 gEff0DType35Lens[6] = {
#include "sub_main/00467868.inc"
};
u32 gEffect0DKeys_70[16] = {
#include "sub_main/00467880.inc"
};
u32 gEffect0DKeys_71[16] = {
#include "sub_main/004678C0.inc"
};
u32 gEffect0DKeys_72[8] = {
#include "sub_main/00467900.inc"
};
s16 gEff0DType36Starts[8] = {
#include "sub_main/00467920.inc"
};
void *gEffect0DKeys[76] = {
    (void *)gEffect0DKeys_0, (void *)gEffect0DKeys_1, (void *)gEffect0DKeys_2, (void *)gEffect0DKeys_3,
    (void *)gEffect0DKeys_4, (void *)gEffect0DKeys_5, (void *)gEffect0DKeys_6, (void *)gEffect0DKeys_7,
    (void *)gEffect0DKeys_8, (void *)gEffect0DKeys_9, (void *)gEffect0DKeys_10, (void *)gEffect0DKeys_11,
    (void *)gEffect0DKeys_12, (void *)gEffect0DKeys_13, (void *)gEffect0DKeys_14, (void *)gEffect0DKeys_15,
    (void *)gEffect0DKeys_16, (void *)gEffect0DKeys_17, (void *)gEffect0DKeys_18, (void *)gEffect0DKeys_19,
    (void *)gEffect0DKeys_20, (void *)gEffect0DKeys_21, (void *)gEffect0DKeys_22, (void *)gEffect0DKeys_23,
    (void *)gEffect0DKeys_24, (void *)gEffect0DKeys_25, (void *)gEffect0DKeys_26, (void *)gEffect0DKeys_27,
    (void *)gEffect0DKeys_28, (void *)gEffect0DKeys_29, (void *)gEffect0DKeys_30, (void *)gEffect0DKeys_31,
    (void *)gEffect0DKeys_32, (void *)gEffect0DKeys_33, (void *)gEffect0DKeys_34, (void *)gEffect0DKeys_35,
    (void *)gEffect0DKeys_36, (void *)gEffect0DKeys_37, (void *)gEffect0DKeys_38, (void *)gEffect0DKeys_39,
    (void *)gEffect0DKeys_40, (void *)gEffect0DKeys_41, (void *)gEffect0DKeys_42, (void *)gEffect0DKeys_43,
    (void *)gEffect0DKeys_44, (void *)gEffect0DKeys_45, (void *)gEffect0DKeys_46, (void *)gEffect0DKeys_47,
    (void *)gEffect0DKeys_46, (void *)gEffect0DKeys_49, (void *)gEffect0DKeys_50, (void *)gEffect0DKeys_51,
    (void *)gEffect0DKeys_50, (void *)gEffect0DKeys_53, (void *)gEffect0DKeys_54, (void *)gEffect0DKeys_55,
    (void *)gEffect0DKeys_56, (void *)gEffect0DKeys_57, (void *)gEffect0DKeys_58, (void *)gEffect0DKeys_59,
    (void *)gEffect0DKeys_60, (void *)gEffect0DKeys_61, (void *)gEffect0DKeys_62, (void *)gEffect0DKeys_63,
    (void *)gEffect0DKeys_62, (void *)gEffect0DKeys_63, (void *)gEffect0DKeys_62, (void *)gEffect0DKeys_63,
    (void *)gEffect0DKeys_68, (void *)gEffect0DKeys_69, (void *)gEffect0DKeys_70, (void *)gEffect0DKeys_71,
    (void *)gEffect0DKeys_72, NULL, NULL, NULL,
};
s16 gEff0DPartCounts[48] = {
#include "sub_main/00467A60.inc"
};
u16 gEff0DPartLens[48] = {
#include "sub_main/00467AC0.inc"
};
s16 gEff0DKeyStarts[48] = {
#include "sub_main/00467B20.inc"
};
s16 gEff0DKeyStrides[48] = {
#include "sub_main/00467B80.inc"
};
s16 gEff0DOnGround[48] = {
#include "sub_main/00467BE0.inc"
};
