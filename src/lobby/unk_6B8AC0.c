#include "common.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#pragma optimization_level 4

void Vec_Copy(f32 *dst, f32 *src);
void VObj_Update(VObj *o);
s32 Gallery_AddObj(VObj *, u8 mode);
extern u8 D_007DCDE8[];

extern u8 gVObjScaleKeys0[];
extern u8 gVObjColorKeys0[];
extern u8 gVObjScaleKeys1[];
extern u8 gVObjColorKeys1[];
extern u8 gVObjScaleKeys2[];
extern u8 gVObjColorKeys2[];
extern u8 gVObjScaleKeys3[];
extern u8 gVObjColorKeys3[];
extern u8 gVObjScaleKeys4[];
extern u8 gVObjColorKeys4[];
extern f32 gCamMatrix[];
void VObj_DrawPicture(Vec3f *pos, u8 kind, u32 ax, u32 ay, s8 *timer);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8, u32);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void VObj_Draw(DrawSlot *dr)
{
    VObj *o = dr->owner;
    Model *mdl = gCommonModels[0];
    VObjData *d = (VObjData *)o->buf;
    ModelMat *mats;
    f32 m[16];
    f32 mt[16];
    f32 scale[3];
    f32 rot[3];
    s32 col;
    s32 i;
    u8 kind;

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        ModelMatSet *mesh;
        u8 *scaleKeys;
        u8 *colKeys;
        mats = mdl->mats;
        col = -1;
        switch (o->type) {
        case 0:
            i = dr->part;
            kind = d->kinds[i];
            if (kind > 0 && o->var0 < 2 && d->finished == 0) {
                VObj_DrawPicture((Vec3f *)dr->pos, kind, d->ang[i], d->heading[i], (s8 *) &d->unk111[i]);
            } else if ((s8)d->unk111[i] != 0) {
                d->unk111[i] = 0;
            }
            break;
        case 1:
            scaleKeys = gVObjScaleKeys0;
            colKeys = gVObjColorKeys0;
            mesh = &mdl->matSets[20];
            break;
        case 2:
            scaleKeys = gVObjScaleKeys1;
            colKeys = gVObjColorKeys1;
            mesh = &mdl->matSets[18];
            break;
        case 3:
            scaleKeys = gVObjScaleKeys2;
            colKeys = gVObjColorKeys2;
            mesh = &mdl->matSets[31];
            Mtx_Translation(mt, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32)mt);
            break;
        case 4:
        case 5:
        case 6:
            scaleKeys = gVObjScaleKeys3;
            colKeys = gVObjColorKeys3;
            mesh = &mdl->matSets[13];
            break;
        case 7:
        case 8:
        case 9:
            scaleKeys = gVObjScaleKeys4;
            colKeys = gVObjColorKeys4;
            mesh = &mdl->matSets[14];
            break;
        }
        if (o->type > 0) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = 6.2831855f * (f32)(u32)d->heading[15] / 65536.0f;
            Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, (f32)o->timer);
            Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, dr->pos, 6, (f32 *) m);
            Mtx_Concat3x3(m, gCamMatrix);
            Color_EvalKeys((s32 *) colKeys, (f32)o->timer, (u32 *) &col);
            Gs_SetState(0x6D, 7);
            Effect_DrawModel(mesh, m, col, 2, mats);
            Gs_SetState(0x6D, 3);
        }
    }
}

void VObj_Spawn(u8 *kinds, u8 *n, Vec3f *pos, f32 f)
{
    Vec3f v = *pos;
    VObj *o;
    VObjData *d;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0x20;
        o->type = 0;
        o->update = VObj_Update;
        d = (VObjData *)o->buf;
        d->kinds = kinds;
        d->spotIdx = n;
        d->delay = d->delayLeft = (s32)f;
        d->pos = v;
        Gallery_AddObj(o, 0);
    }
}

void VObj_GetPartPos(Vec3f *out, VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    *out = *(Vec3f *)d->parts[i]->pos;
}

void VObj_SetFlag(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    d->flag[i] = 1;
}

u8 VObj_GetFlag(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    return d->flag[i];
}

s32 VObj_IsFinished(VObj *o)
{
    VObjData *d = (VObjData *)o->buf;

    if (o->var0 < 2 && d->finished == 0) {
        return 0;
    }
    return 1;
}

void VObj_SpawnGroup(Vec3f *pos)
{
    u8 i;
    VObj *o;

    for (i = 1; i < 7; i++) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x20;
            o->type = i;
            o->update = VObj_Update;
            Vec_Copy((f32 *) &o->pos, (f32 *) pos);
        }
    }
}

void VObj_SpawnList(Vec3f *pos)
{
    u8 i;
    u8 *k = D_007DCDE8;
    VObj *o;

    for (i = 0; i < 5; i++, k++) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x20;
            o->type = *k;
            o->update = VObj_Update;
            Vec_Copy((f32 *) &o->pos, (f32 *) pos);
        }
    }
}

u8 VObj_GetPartSizeKind(VObj *o, u8 i)
{
    return gVObjKinds[((VObjData *)o->buf)->kinds[i]].unk5;
}

u16 func_006B9040(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    return d->heading[i];
}

f32 VObj_GetPartAngle(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    return 6.2831855f * (u32)d->ang[i] / 65536.0f;
}

u16 VObj_GetPartSe(VObj *o, u8 i)
{
    return gVObjKinds[((VObjData *)o->buf)->kinds[i]].unk0;
}

s32 VObj_TickTimer(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    return --d->timer[i] <= 0;
}

s16 VObj_GetPoints(VObj *o, u8 i)
{
    u8 *k = ((VObjData *)o->buf)->kinds;
    VKind *t = gVObjKinds;

    k = &k[i];
    return t[*k].unk2;
}

/* unk_6B8AC0's .data (0x7DCBB0-0x7DCDF0) */
u8 gVObjScaleKeys0[64] = {
#include "lobby/007DCBB0.inc"
};
u8 gVObjColorKeys0[32] = {
#include "lobby/007DCBF0.inc"
};
u8 gVObjScaleKeys1[96] = {
#include "lobby/007DCC10.inc"
};
u8 gVObjColorKeys1[48] = {
#include "lobby/007DCC70.inc"
};
u8 gVObjScaleKeys2[96] = {
#include "lobby/007DCCA0.inc"
};
u8 gVObjColorKeys2[48] = {
#include "lobby/007DCD00.inc"
};
u8 gVObjScaleKeys3[64] = {
#include "lobby/007DCD30.inc"
};
u8 gVObjColorKeys3[32] = {
#include "lobby/007DCD70.inc"
};
u8 gVObjScaleKeys4[48] = {
#include "lobby/007DCD90.inc"
};
u8 gVObjColorKeys4[40] = {
#include "lobby/007DCDC0.inc"
};
u8 D_007DCDE8[8] = {
#include "lobby/007DCDE8.inc"
};
