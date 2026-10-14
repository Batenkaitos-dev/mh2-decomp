/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
/* Level 4: VObj_DrawPicture matches only there; the rest match at both. */
#pragma optimization_level 4
void *memset(void *, s32, u32);


void VPart_Init(VObj *);
void VPart_Run(VObj *o);
void VPart_End(VObj *);
void VPart_Free(VObj *o);
void VDigits_Init(VObj *o);
void VDigits_Run(VObj *o);
void VDigits_End(VObj *);
void VDigits_Free(VObj *o);
void VFx_Init(VObj *o);
void VFx_Run(VObj *o);
void VFx_End(VObj *o);
void VFx_Free(VObj *o);
void VObj_SetHit(VObj *o);
void VDigits_Spawn(s16 arg0, u16 arg1);
void VPart_Spawn(void *arg0);
void VNumber_Init(VObj *o);
void VNumber_Run(VObj *o);
void VNumber_End(VObj *o);
void VNumber_Free(VObj *o);
void MealScene_Close(void);
void VMuzzle_Init(VObj *o);
void VMuzzle_Run(VObj *o);
void VMuzzle_End(VObj *o);
void VMuzzle_Free(VObj *o);
void DrawSlot_Free(s32);
void VDigits_Update(VObj *);
void VPart_Update(VObj *);
void CamFocus_End(void);
void CamReq_Cancel(void);
void Npc_EnterRoutePoint(Em *npc);
void LobbyMenu_Init(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s32 gMenuWork;
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 Gallery_InMenuOrClose(void);
s32 Gallery_InResults(void);
void LobbySe_Play(s32);

u8 VObj_IsCleared(VObj *o)
{
    return ((VObjData *)o->buf)->finished;
}

void VObj_ResetTimer(VObj *o, u8 i, u8 k)
{
    VObjData *d = (VObjData *)o->buf;

    d->timer[i] = gVObjKinds[k].timer;
}

void func_006B91A0(VObj *o, u8 i)
{
    VObjData *d = (VObjData *)o->buf;

    d->unk111[i] = 0x19;
}

typedef struct Mtx44 {
    f32 m[4][4];
} Mtx44;
void Mtx_Identity(f32 *m);
s32 Gs_SetState(u8, u32);
void Camera_ProjectPoint(f32 *out, f32 *v);
#include "prim.h"
void Gfx_SetState63(s32);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
f32 Math_Sin(f32 a);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_RotateY(f32 *v, f32 a);
s32 EntryPicture_GetUVf(f32 *uv0, f32 *uv1, u16 id);
s32 Prim_DrawTexTriF(PrimTexTriF *t);
extern f32 gVObjSwingAmps[];
extern f32 gVObjHeights[];

/* Draws a member as a textured quad (two triangles) at pos, turned by ax
 * and ay; while *timer runs it swings about x (gVObjSwingAmps: the amplitudes). */
void VObj_DrawPicture(Vec3f *pos, u8 kind, u32 ax, u32 ay, s8 *timer)
{
    Vec3f v0;
    Vec3f v1;
    Vec3f v2;
    Vec3f v3;
    f32 uv[2];
    f32 uv2[2];
    Mtx44 m;
    PrimTexTriF prim;
    f32 ry;
    s8 t;
    f32 hi;
    f32 rx;
    f32 lo;
    f32 w;
    f32 off;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x147);
    Tex_Bind(0x147);
    w = gVObjHeights[gVObjKinds[kind].unk5];
    lo = -w / 2.0f;
    hi = w / 2.0f;
    v0.x = lo;
    v0.y = hi;
    v0.z = 0.0f;
    off = 0.0f;
    t = *timer;
    if (t <= 0) {
        *timer = 0;
    } else {
        ax = 0;
        off = 1.3962635f * (-1.0f * gVObjSwingAmps[(u8)((t - 1) / 5.0f)]) *
              Math_Sin(6.2831855f * (72.0f * (25 - t)) / 360.0f);
        (*timer)--;
    }
    rx = 6.2831855f * ax / 65536.0f + off;
    Vec_RotateX(&v0.x, rx);
    ry = 6.2831855f * ay / 65536.0f;
    Vec_RotateY((f32 *)&v0, ry);
    v0.x += pos->x;
    v0.y += pos->y;
    v0.z += pos->z;
    v1.x = hi;
    v1.y = hi;
    v1.z = 0.0f;
    Vec_RotateX(&v1.x, rx);
    Vec_RotateY((f32 *)&v1, ry);
    v1.x += pos->x;
    v1.y += pos->y;
    v1.z += pos->z;
    v2.x = lo;
    v2.y = lo;
    v2.z = 0.0f;
    Vec_RotateY((f32 *)&v2, ry);
    v2.x += pos->x;
    v2.y += pos->y;
    v2.z += pos->z;
    v3.x = hi;
    v3.y = lo;
    v3.z = 0.0f;
    Vec_RotateY((f32 *)&v3, ry);
    v3.x += pos->x;
    v3.y += pos->y;
    v3.z += pos->z;
    prim.color[0] = -1;
    prim.color[1] = -1;
    prim.color[2] = -1;
    EntryPicture_GetUVf(uv, uv2, gVObjKinds[kind].unk0);
    prim.st[0][0] = uv[0];
    prim.st[0][1] = uv[1];
    prim.st[1][0] = uv[0] + 0.1640625f;
    prim.st[1][1] = uv[1];
    prim.st[2][0] = uv[0];
    prim.st[2][1] = uv[1] + 0.1640625f;
    Mtx_Identity((f32 *) &m);
    Gs_SetState(0x1A, (u32)&m);
    Camera_ProjectPoint((f32 *) &prim.v[0], (f32 *) &v0);
    prim.v[0].q = 1.0f;
    Camera_ProjectPoint((f32 *) &prim.v[1], (f32 *) &v1);
    prim.v[1].q = 1.0f;
    Camera_ProjectPoint((f32 *) &prim.v[2], (f32 *) &v2);
    prim.v[2].q = 1.0f;
    Prim_DrawTexTriF(&prim);
    prim.st[0][0] = uv2[0];
    prim.st[0][1] = uv2[1];
    Camera_ProjectPoint((f32 *) &prim.v[0], (f32 *) &v3);
    prim.v[0].q = 1.0f;
    Prim_DrawTexTriF(&prim);
}

void VFx_Update(VObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        VFx_Init(o);
        break;
    case 1:
        VFx_Run(o);
        break;
    case 2:
        VFx_End(o);
        break;
    case 3:
        VFx_Free(o);
        break;
    }
}

/* a village effect (type 0x21, VFx_Update): kind 0 a shot flying along vel, kinds 1-4 a burst that grows and fades */
typedef struct VFx {
    /* 0x00 */ f32 speed;
    /* 0x04 */ Vec3f pos;
    /* 0x10 */ Vec3f target;
    /* 0x1C */ Vec3f pos1;
    /* 0x28 */ Vec3f pos2;
    /* 0x34 */ Vec3f vel;
    /* 0x40 */ u8 unk40;
    /* 0x41 */ u8 unk41[3];
    /* 0x44 */ Vec3f scale;      /* Key_LerpVec3 */
    /* 0x50 */ f32 alpha;         /* Key_LerpF32 */
    /* 0x54 */ s16 rand;
} VFx;
extern s8 D_007DCE97[];
f32 Lure_RandSigned(void);
void Vec_RotateY(f32 *v, f32 a);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
u16 Rand_Next(s32 stream);
void VFx_Draw(DrawSlot *);

void VFx_Init(VObj *o)
{
    VFx *w = (VFx *)o->buf;
    Vec3f v;
    f32 f;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->state = 1;
    o->visible = 1;
    o->draw = 0;
    o->timer = 0;
    switch (o->type) {
    case 0:
        w->unk40 = 0;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        w->rand = Rand_Next(1);
        o->timer = D_007DCE97[o->type];
        if (o->type > 1) {
            v.x = 1.0f;
            v.y = 0.0f;
            v.z = 0.0f;
            Vec_RotateY((f32 *)&v, (f32)(u32)o->angle2 * 6.2831855f / 65536.0f);
            f = 15.0f * Lure_RandSigned();
            o->pos[0] += v.x * f;
            o->pos[1] += 15.0f * Lure_RandSigned();
            w->speed = 1.0f;
        }
        break;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = VFx_Draw;
    } else {
        StageObj_Free(o);
    }
}

extern u8 gVFxAlphaKeys0[];
extern u8 gVFxScaleKeys0[];
extern u8 gVFxAlphaKeys1[];
extern u8 gVFxScaleKeys1[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gallery_AddObj(VObj *, u8 mode);
void Vec_Copy(f32 *dst, f32 *src);

void VFx_Run(VObj *o)
{
    VFx *w = (VFx *)o->buf;
    u8 *fc;
    u8 *vc;

    switch (o->type) {
    case 0:
        if (w->unk40 == 0) {
            w->pos2 = w->pos1;
            w->pos1.x += w->vel.x;
            w->pos1.y += w->vel.y;
            w->pos1.z += w->vel.z;
            Vec_Copy(o->node->pos, (f32 *)&w->pos1);
            if (o->timer++ > 0x3C) {
                o->state = 2;
            }
            OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
            Gallery_AddObj(o, 1);
        } else {
            o->state = 2;
        }
        break;
    case 1:
        fc = gVFxAlphaKeys0;
        vc = gVFxScaleKeys0;
        if (o->timer++ > 4) {
            o->state++;
        } else {
            Key_LerpVec3((f32 *) vc, (f32 *) &w->scale, o->timer);
            Key_LerpF32((f32 *) fc, (f32 *) &w->alpha, o->timer);
            Vec_Copy(o->node->pos, (f32 *)&o->pos);
            OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        }
        break;
    case 2:
    case 3:
    case 4:
        if (++o->timer < 0) {
            break;
        }
        fc = gVFxAlphaKeys1;
        vc = gVFxScaleKeys1;
        if (o->timer > 0x1E) {
            o->state++;
        } else {
            Key_LerpVec3((f32 *) vc, (f32 *) &w->scale, o->timer);
            Key_LerpF32((f32 *) fc, (f32 *) &w->alpha, o->timer);
            o->pos[1] += w->speed;
            Vec_Copy(o->node->pos, (f32 *)&o->pos);
            OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        }
        break;
    }
}

void VFx_End(VObj *o)
{
    o->state = 3;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void VFx_Free(VObj *o)
{
    StageObj_Free(o);
}

void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern f32 gCamMatrix[];

void VFx_Draw(DrawSlot *slot)
{
    VObj *o = slot->owner;
    Model *mdl = gCommonModels[0];
    VFx *w = (VFx *)o->buf;
    ModelMat *tex;
    ModelMatSet *set;
    u16 flags = 0;
    u32 color;
    Vec3f scale;
    Vec3f rot;
    f32 *rz;
    f32 m[16];

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        tex = mdl->mats;
        rz = &rot.z;
        rot.x = 0.0f;
        rot.y = 0.0f;
        *rz = 0.0f;
        switch (o->type) {
        case 0:
            color = -1;
            set = &mdl->matSets[39];
            scale.x = 1.0f;
            scale.y = 1.0f;
            scale.z = 1.0f;
            break;
        case 1:
            set = &mdl->matSets[1];
            flags |= 2;
            scale.x = w->scale.x;
            scale.y = w->scale.y;
            scale.z = w->scale.z;
            *rz = 6.2831855f * w->rand / 65536.0f;
            color = ((u8)(255.0f * w->alpha) << 24) | 0xFFFFFF;
            break;
        case 2:
        case 3:
        case 4:
            set = &mdl->matSets[6];
            flags |= 4;
            scale.x = w->scale.x;
            scale.y = w->scale.y;
            scale.z = w->scale.z;
            *rz = 6.2831855f * w->rand / 65536.0f;
            color = ((u8)(255.0f * w->alpha) << 24) | 0x606060;
            break;
        }
        Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) m);
        Mtx_Concat3x3(m, gCamMatrix);
        Effect_DrawModel(set, m, color, flags, tex);
    }
}

void Vec_Normalize(f32 *v);

void VFx_SpawnShot(Vec3f *from, Vec3f *to)
{
    VObj *o;
    VFx *w;
    Vec3f v;
    Vec3f t;
    f32 d = 155.0f;
    f32 *vy;
    f32 *vz;

    t.x = to->x;
    t.y = to->y;
    t.z = to->z;
    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0x21;
        o->type = 0;
        o->update = VFx_Update;
        vy = &v.y;
        vz = &v.z;
        w = (VFx *)o->buf;
        v.x = to->x - from->x;
        *vy = to->y - from->y;
        *vz = to->z - from->z;
        Vec_Normalize((f32 *)&v);
        w->pos.x = from->x + v.x * d;
        w->pos.y = from->y + *vy * d;
        w->pos.z = from->z + *vz * d;
        w->target = t;
        w->pos1 = w->pos;
        w->pos2 = w->pos1;
        w->speed = 50.0f;
        w->vel.x = v.x * w->speed;
        w->vel.y = *vy * w->speed;
        w->vel.z = *vz * w->speed;
    }
}

void Vec_Normalize(f32 *v);
struct HitCapsule;
struct Capsule;
void Capsule_Copy(struct HitCapsule *src, struct Capsule *dst);
typedef struct VObjCam {
    u8 unk0[0x1C];
    Vec3f b;
    Vec3f a;
    Vec3f v;
    u8 unk40;               /* set by VObj_SetHit */
} VObjCam;
void VObj_GetCapsule(f32 *out, f32 *a, VObj *o, f32 f)
{
    VObjCam *d = (VObjCam *)o->buf;
    Vec3f v = d->v;

    Vec_Normalize((f32 *)&v);
    out[6] = f;
    out[0] = d->a.x;
    out[1] = d->a.y;
    out[2] = d->a.z;
    out[3] = d->b.x;
    out[4] = d->b.y;
    out[5] = d->b.z;
    Capsule_Copy((struct HitCapsule *) out, (struct Capsule *) a);
}

void VObj_SetHit(VObj *o)
{
    ((VObjCam *)o->buf)->unk40 = 1;
}

u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
void Vec_RotateY(f32 *v, f32 a);

void VFx_SpawnRing(Vec3f *pos, Vec3f *dir)
{
    VObj *o;
    u8 k;
    Vec3f v;
    f32 *vy = &v.y;
    f32 *vz = &v.z;

    for (k = 1; k < 5; k++) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            o->kind = 0x21;
            o->type = k;
            o->update = VFx_Update;
            *(Vec3f *)o->pos = *pos;
            o->angle2 = Math_Heading(dir->x, dir->z, 0.0f, 0.0f);
            v.x = 25.0f;
            *vy = 0.0f;
            *vz = 0.0f;
            Vec_RotateY((f32 *)&v, (f32)o->angle2 * 6.2831855f / 65536.0f);
            o->pos[0] += v.x;
            o->pos[1] += *vy;
            o->pos[2] += *vz;
        }
    }
}

void VDigits_Update(VObj *o)
{
    switch (o->state) {
    case 0:
        VDigits_Init(o);
        break;
    case 1:
        VDigits_Run(o);
        break;
    case 2:
        VDigits_End(o);
        break;
    case 3:
        VDigits_Free(o);
        break;
    }
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void VDigits_Draw(DrawSlot *slot);
/* The data of a number shown with digit sprites (type 0x23): the digits, lowest first. */
typedef struct VDigits {
    /* 0x00 */ s16 digit[10];
    /* 0x14 */ s16 value;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 count;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ s16 y;
} VDigits;
void VDigits_Init(VObj *o)
{
    VDigits *d = (VDigits *)o->buf;
    s32 i;
    s32 n;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->state = 1;
    o->visible = 1;
    o->draw = 0;
    o->timer = 0;
    o->slot = DrawSlot_Alloc();
    n = d->value;
    if (n > 0) {
        for (i = 0; i < 10; i++) {
            d->digit[i] = n % 10;
            n -= d->digit[i];
            n /= 10;
            if (n == 0) {
                break;
            }
        }
        d->count = i + 1;
    } else {
        d->digit[0] = 0;
        d->count = 1;
    }
    d->unk16 = 0;
    d->unk18 = 0;
    d->unk1C = 0;
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = VDigits_Draw;
    } else {
        StageObj_Free(o);
    }
}

void VDigits_Run(VObj *o)
{
    s16 timer;
    s16 temp_v0_2;
    VDigits *d;

    d = (VDigits *)o->buf;
    if (Gallery_InResults() == 1) {
        d->unk16 = (s16) d->count;
        o->timer = 0;
    }
    if (d->unk16 < d->count) {
        timer = o->timer + 1;
        o->timer = timer;
        if (timer > 30) {
            d->unk16 = (s16) (d->unk16 + 1);
            o->timer = 0;
            d->unk18 = 0;
            LobbySe_Play(0xA);
        } else {
            temp_v0_2 = d->unk1C + 1;
            d->unk1C = temp_v0_2;
            if (temp_v0_2 >= 2) {
                d->unk18 = (s16) (d->unk18 + 1);
                d->unk18 = (s16) ((s16) d->unk18 % 10);
                LobbySe_Play(1);
                d->unk1C = 0;
            }
        }
        OT_AddAt(gDrawOtSorted, o->node, 0, 1);
        return;
    }
    if (Gallery_InMenuOrClose() == 1) {
        o->state = 2;
        return;
    }
    OT_AddAt(gDrawOtSorted, o->node, 0, 1);
}

void VDigits_End(VObj *o)
{
    o->state = 3;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void VDigits_Free(VObj *o)
{
    StageObj_Free(o);
}

void Text_SetSize(s32 size);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *s, ...);
void VDigits_Draw(DrawSlot *slot)
{
    VObj *o = slot->owner;
    VDigits *d = (VDigits *)o->buf;
    s32 i;

    if (o->visible) {
        Text_SetSize(0);
        Font_SetColor(0x80101010);
        Font_SetSpacing(0x14, 0x14);
        if (o->timer <= 0) {
            s32 n;
            n = d->unk16;
            if (n >= d->count) {
                n--;
            }
            for (i = 0; i <= n; i++) {
                Font_SetPos(0x1B3 - i * 15, d->y);
                Text_PrintfColored("%d", d->digit[i]);
            }
        } else {

            for (i = 0; i < d->unk16; i++) {
                Font_SetPos(0x1B3 - i * 15, d->y);
                Text_PrintfColored("%d", d->digit[i]);
            }
            Font_SetPos(0x1B3 - d->unk16 * 15, d->y);
            Text_PrintfColored("%d", d->unk18);
        }
    }
}

void VDigits_Spawn(s16 points, u16 y)
{
    VObj *o;
    VDigits *d;

    o = StageObj_Alloc(1);
    if (o != NULL) {
        o->kind = 0x22;
        o->type = 0;
        o->update = VDigits_Update;
        d = (VDigits *)o->buf;
        d->value = points;
        d->y = y;
    }
}

void VPart_Update(VObj *o)
{
    switch (o->state) {
    case 0:
        VPart_Init(o);
        break;
    case 1:
        VPart_Run(o);
        break;
    case 2:
        VPart_End(o);
        break;
    case 3:
        VPart_Free(o);
        break;
    }
}

/*
 * The data of a village particle object (VPart_Update's states): six parts
 * (draw slots drawn by VPart_Draw) thrown from a bone of the monster in
 * VObj.unk34. Kind 0: sparks from bone 0xC or 0x14, re-thrown on a timer while
 * `on` (toggled at random); kind 1: tumbling debris that falls, until the
 * monster's motion 0x43D passes frame 50; kind 2: six pieces at bones 6 and 9.
 */
typedef struct VPartFx {
    /* 0x000 */ Em *em;
    /* 0x004 */ Vec3f dir[6];       /* unit direction (z turned by random X and Y angles) */
    /* 0x04C */ Vec3f pos[6];
    /* 0x094 */ Vec3f rot[6];       /* degrees (VPart_Draw) */
    /* 0x0DC */ Vec3f spin[6];      /* added to rot every frame */
    /* 0x124 */ f32 speed[6];
    /* 0x13C */ s8 timer[6];        /* frames until the part is thrown again */
    /* 0x142 */ s8 frame[6];        /* frames since thrown (VPart_Draw's key time) */
    /* 0x148 */ f32 vy[6];          /* kind 1: vertical speed (gravity 1, down to -2) */
    /* 0x160 */ s16 slot[6];        /* DrawSlot_Alloc */
    /* 0x16C */ DrawSlot *part[6];     /* DrawSlot_Get(slot); NULL: none was free */
    /* 0x184 */ u8 on;
} VPartFx;
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 Npc_IsBusyNotAct3C4B(Em *arg0);
f32 Lure_RandUnit(void);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void Vec_RotateX(f32 *v, f32 angle);

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void VPart_Draw(DrawSlot *slot);

/* Init (state 0) of the particle objects: throws the six parts (kinds 0 and
 * 1 from a bone, kind 2 at bones 6 and 9), gives each a draw slot, then
 * runs the first frame. */
void VPart_Init(VObj *o)
{
    f32 m[4][4];
    Vec3f v;
    VPartFx *d = (VPartFx *)o->buf;
    u8 i;
    s32 a;
    s32 b;
    s8 sign;
    s8 r;
    f32 x;
    f32 y;
    f32 z;

    o->state = 1;
    o->visible = 1;
    o->draw = 0;
    if (o->type == 2) {
        o->timer = 30;
    } else {
        o->timer = (u16)Rand_Next(1) % 20 + 10;
    }
    d->on = 1;
    for (i = 0; i < 6; i++) {
        switch (o->type) {
        case 0:
            if (Npc_IsBusyNotAct3C4B(o->owner) != 0) {
                Em_GetBonePos(o->owner, 0xC, &d->pos[i].x);
            } else {
                Em_GetBonePos(o->owner, 0x14, &d->pos[i].x);
            }
            d->timer[i] = (u16)Rand_Next(1) % 3 + 6;
            d->speed[i] = 12.0f + (f32)((u16)Rand_Next(1) % 4);
            d->dir[i].x = 0.0f;
            d->dir[i].y = 0.0f;
            d->dir[i].z = 1.0f;
            a = (u16)Rand_Next(1) % 32767 + 0x8000;
            b = (u16)Rand_Next(1);
            Vec_RotateX(&d->dir[i].x, 6.2831855f * a / 65536.0f);
            Vec_RotateY((f32 *)&d->dir[i], 6.2831855f * b / 65536.0f);
            break;
        case 1:
            if (Npc_IsBusyNotAct3C4B(o->owner) != 0) {
                Em_GetBonePos(o->owner, 0xC, &d->pos[i].x);
            } else {
                Em_GetBonePos(o->owner, 0x14, &d->pos[i].x);
            }
            d->vy[i] = 8.0f + 6.0f * Lure_RandUnit();
            d->pos[i].y += 15.0f;
            d->timer[i] = 60.0f * Lure_RandUnit();
            d->speed[i] = 3.0f + (f32)((u16)Rand_Next(1) % 3);
            d->rot[i].x = 360.0f * Lure_RandUnit();
            d->rot[i].y = 360.0f * Lure_RandUnit();
            d->rot[i].z = 360.0f * Lure_RandUnit();
            d->frame[i] = 0;
            r = (s8)Rand_Next(1) % 2;
            sign = r == 0 ? 1 : -1;
            x = 30.0f + 15.0f * (sign * Lure_RandUnit());
            y = 30.0f + 15.0f * (sign * Lure_RandUnit());
            z = 30.0f + 15.0f * (sign * Lure_RandUnit());
            d->spin[i].x = x / d->timer[i];
            d->spin[i].y = y / d->timer[i];
            d->spin[i].z = z / d->timer[i];
            d->dir[i].x = 0.0f;
            d->dir[i].y = 0.0f;
            d->dir[i].z = 1.0f;
            a = (u16)Rand_Next(1) % 32767 + 0x8000;
            b = (u16)Rand_Next(1);
            Vec_RotateX(&d->dir[i].x, 6.2831855f * a / 65536.0f);
            Vec_RotateY((f32 *)&d->dir[i], 6.2831855f * b / 65536.0f);
            break;
        case 2:
            d->frame[i] = -1;
            switch (i) {
            case 0:
            case 2:
            case 4:
                Mtx_Copy(m[0], (f32 *)Em_GetBone(d->em, 6));
                v.x = 20.0f;
                v.y = 0.0f;
                v.z = 0.0f;
                break;
            case 1:
            case 3:
            case 5:
                v.x = -20.0f;
                v.y = 0.0f;
                v.z = 0.0f;
                Mtx_Copy(m[0], (f32 *)Em_GetBone(d->em, 9));
                break;
            }
            Vec_RotateSelf(&v.x, m[0]);
            Mtx_GetTrans(&d->pos[i].x, m[0]);
            d->pos[i].x += v.x;
            d->pos[i].y += v.y;
            d->pos[i].z += v.z;
            switch (i) {
            case 0:
            case 1:
                d->timer[i] = 24;
                break;
            case 2:
            case 3:
                d->timer[i] = 30;
                d->rot[i].z = 360.0f * Lure_RandUnit();
                d->spin[i].z = 120.0f * Lure_RandSigned() / d->timer[i];
                break;
            case 4:
            case 5:
                d->timer[i] = 16;
                break;
            }
            break;
        }
        d->slot[i] = DrawSlot_Alloc();
        if (d->slot[i] != -1) {
            d->part[i] = DrawSlot_Get(d->slot[i]);
            d->part[i]->owner = o;
            d->part[i]->part = i;
            d->part[i]->draw = VPart_Draw;
        } else {
            d->part[i] = NULL;
        }
    }
    VPart_Run(o);
}

void VPart_Run(VObj *o)
{
    VPartFx *d = (VPartFx *)o->buf;
    u8 i;
    s32 a;
    s32 b;
    s8 sign;
    s8 r;
    f32 x;
    f32 y;
    f32 z;

    if (o->type == 0) {
        if (o->timer-- <= 0) {
            d->on ^= 1;
            o->timer = (u16)Rand_Next(1) % 20 + 10;
        }
    }
    if (d->on == 1) {
        for (i = 0; i < 6; i++) {
            if (d->part[i] != NULL) {
                switch (o->type) {
                case 0:
                case 1:
                    d->pos[i].x += d->dir[i].x * d->speed[i];
                    d->pos[i].z += d->dir[i].z * d->speed[i];
                    d->frame[i]++;
                    if (o->type == 1) {
                        if (d->vy[i] <= 0.0f) {
                            d->vy[i] = -2.0f;
                            if (!(d->speed[i] < 3.0f)) {
                                d->speed[i] -= 2.0f;
                            }
                        } else {
                            d->vy[i] += -1.0f;
                        }
                        d->pos[i].y += d->vy[i];
                        d->rot[i].x += d->spin[i].x;
                        d->rot[i].y += d->spin[i].y;
                        d->rot[i].z += d->spin[i].z;
                    } else {
                        d->pos[i].y += d->dir[i].y * d->speed[i];
                    }
                    if (d->timer[i]-- <= 0) {
                        d->dir[i].x = 0.0f;
                        d->dir[i].y = 0.0f;
                        d->dir[i].z = 1.0f;
                        a = (u16)Rand_Next(1) % 32767 + 0x8000;
                        b = (u16)Rand_Next(1);
                        Vec_RotateX(&d->dir[i].x, 6.2831855f * a / 65536.0f);
                        Vec_RotateY((f32 *)&d->dir[i], 6.2831855f * b / 65536.0f);
                        if (Npc_IsBusyNotAct3C4B(o->owner) != 0) {
                            Em_GetBonePos(o->owner, 0xC, &d->pos[i].x);
                        } else {
                            Em_GetBonePos(o->owner, 0x14, &d->pos[i].x);
                        }
                        switch (o->type) {
                        case 0:
                            d->speed[i] = 12.0f + (f32)((u16)Rand_Next(1) % 4);
                            d->timer[i] = 6.0f + (f32)((u16)Rand_Next(1) % 3);
                            break;
                        case 1:
                            d->frame[i] = 0;
                            d->speed[i] = 3.0f + (f32)((u16)Rand_Next(1) % 3);
                            d->timer[i] = 60.0f * Lure_RandUnit();
                            d->pos[i].y += 15.0f;
                            d->vy[i] = 8.0f + 6.0f * Lure_RandUnit();
                            d->rot[i].x = 360.0f * Lure_RandUnit();
                            d->rot[i].y = 360.0f * Lure_RandUnit();
                            d->rot[i].z = 360.0f * Lure_RandUnit();
                            r = (s8)Rand_Next(1) % 2; sign = r == 0 ? 1 : -1;
                            x = 30.0f + 15.0f * (sign * Lure_RandUnit());
                            y = 30.0f + 15.0f * (sign * Lure_RandUnit());
                            z = 30.0f + 15.0f * (sign * Lure_RandUnit());
                            d->spin[i].x = x / 60.0f;
                            d->spin[i].y = y / 60.0f;
                            d->spin[i].z = z / 60.0f;
                            break;
                        }
                    }
                    break;
                case 2:
                    d->frame[i]++;
                    if (i == 2 || i == 3) {
                        d->rot[i].z += d->spin[i].z;
                    }
                    break;
                }
                if (o->type != 2 || d->frame[i] <= d->timer[i]) {
                    Vec_Copy(d->part[i]->pos, (f32 *)&d->pos[i]);
                    OT_AddByDepth(gDrawOtSorted, d->part[i], 0x40, 0);
                }
            }
        }
    }
    switch (o->type) {
    case 0:
        if (((Em *)o->owner)->state != 0x6F) {
            o->state = 2;
        }
        break;
    case 1:
        if (d->em->motionId[0] == 0x43D && Motion_PassedFrame(d->em, 0, 50.0f) != 0) {
            o->state = 2;
        }
        break;
    case 2:
        if (o->timer-- <= 0) {
            o->state = 2;
        }
        break;
    }
}

void VPart_End(VObj *o)
{
    VPartFx *d = (VPartFx *)o->buf;
    u8 i;

    o->state++;
    for (i = 0; i < 6; i++) {
        if (d->part[i] != NULL) {
            DrawSlot_Free(d->slot[i]);
        }
    }
}

void VPart_Free(VObj *o)
{
    StageObj_Free(o);
}

extern u8 gVPartAlphaKeys[];
extern u8 gVPartScaleKeys0[];
extern u8 gVPartColorKeys0[];
extern u8 gVPartScaleKeys1[];
extern u8 gVPartColorKeys1[];
extern u8 gVPartScaleKeys2[];
extern u8 gVPartColorKeys2[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);

void VPart_Draw(DrawSlot *slot)
{
    VObj *o = slot->owner;
    Model *mdl = gCommonModels[0];
    VPartFx *d = (VPartFx *)o->buf;
    ModelMat *tex;
    ModelMatSet *set;
    u16 flags = 0;
    u8 *vc;
    u8 *cc;
    u32 rgb;
    f32 m[16];
    Vec3f rot;
    u32 color;
    Vec3f scale;
    f32 alpha;

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        tex = mdl->mats;
        switch (o->type) {
        case 0:
            rot.x = 0.0f;
            rot.y = 0.0f;
            rot.z = 0.0f;
            scale.x = 0.3f;
            scale.y = 0.3f;
            scale.z = 0.3f;
            set = &mdl->matSets[37];
            color = 0xB2FFFFFF;
            flags |= 2;
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 1:
            rot.x = 6.2831855f * d->rot[slot->part].x / 360.0f;
            rot.y = 6.2831855f * d->rot[slot->part].y / 360.0f;
            rot.z = 6.2831855f * d->rot[slot->part].z / 360.0f;
            scale.x = 2.0f;
            scale.y = 2.0f;
            scale.z = 2.0f;
            set = &mdl->matSets[35];
            Key_LerpF32((f32 *) gVPartAlphaKeys, (f32 *) &alpha, d->frame[slot->part]);
            rgb = *(u32 *)((u8 *)o->owner + 0x580);
            color = ((u8)(255.0f * alpha) << 24) | ((u8)(rgb >> 16) << 16) | ((u8)(rgb >> 8) << 8) | (u8)rgb;
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) m);
            break;
        case 2:
            switch (slot->part) {
            case 0:
            case 1:
                vc = gVPartScaleKeys0;
                cc = gVPartColorKeys0;
                set = &mdl->matSets[5];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 0.0f;
                break;
            case 2:
            case 3:
                vc = gVPartScaleKeys1;
                cc = gVPartColorKeys1;
                set = &mdl->matSets[13];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 6.2831855f * d->rot[slot->part].z / 360.0f;
                break;
            case 4:
            case 5:
                vc = gVPartScaleKeys2;
                cc = gVPartColorKeys2;
                set = &mdl->matSets[18];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 0.0f;
                break;
            }
            flags |= 2;
            Key_LerpVec3((f32 *) vc, (f32 *) &scale, d->frame[slot->part]);
            Color_EvalKeys((s32 *) cc, d->frame[slot->part], &color);
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        }
        Effect_DrawModel(set, m, color, flags, tex);
    }
}

void VPart_Spawn(void *obj)
{
    VObj *o;

    o = StageObj_Alloc(1);
    if (o != NULL) {
        o->kind = 0x23;
        o->update = VPart_Update;
        o->owner = obj;
        o->type = 0;
    }
}

void VPart_SpawnKind(void *a, void *b, u8 k)
{
    VObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x23;
        o->update = VPart_Update;
        o->owner = a;
        o->type = k;
        *(s32 *)o->buf = (s32) b;
    }
}

void VNumber_Update(VObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        VNumber_Init(o);
        break;
    case 1:
        VNumber_Run(o);
        break;
    case 2:
        VNumber_End(o);
        break;
    case 3:
        VNumber_Free(o);
        break;
    }
}

void VNumber_Draw(DrawSlot *slot);
void VNumber_Init(VObj *o)
{
    s32 *d = (s32 *)o->buf;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->state = 1;
    o->visible = 1;
    o->draw = 0;
    o->timer = 6;
    ((s16 *)d)[9] = 0;
    d[0] = 0;
    d[1] = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = VNumber_Draw;
    } else {
        StageObj_Free(o);
    }
}

/* The data of a number object (type 0x24): its screen position, the number, a frame count. */
typedef struct VNumber {
    /* 0x00 */ f32 x, y, z;
    /* 0x0C */ u8 unkC[4];
    /* 0x10 */ s16 value;
    /* 0x12 */ u16 t;
} VNumber;
void VNumber_Run(VObj *o)
{
    Mtx44 m;
    VNumber *d = (VNumber *)o->buf;

    if (o->timer-- <= 0) {
        o->state = 2;
    } else {
        d->t++;
        Mtx_Identity((f32 *) &m);
        Gs_SetState(0x1A, (u32)&m);
        Camera_ProjectPoint((f32 *) d, (f32 *) &o->pos);
        d->y -= d->t * 20;
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
    }
}

void VNumber_End(VObj *o)
{
    o->state = 3;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void VNumber_Free(VObj *o)
{
    StageObj_Free(o);
}

void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
/* The file has two "%d" (VDigits_Draw's first): the game has a copy per unit
 * (this file is probably two of them), so this one is an object of its own. */
const char D_00825EE8[3] = {
#include "lobby/00825EE8.inc"
};
void VNumber_Draw(DrawSlot *slot)
{
    VObj *o = slot->owner;
    f32 *d = (f32 *)o->buf;
    f32 x;

    if (o->visible) {
        x = 640.0f * d[0] / 512.0f;
        Font_SetSpacing(0x28, 0x28);
        Text_PrintfAt(-15.0f + x, 7.0f + d[1], 5, D_00825EE8, ((s16 *)d)[8]);
    }
}

void VNumber_Update(VObj *o);
void VNumber_Spawn(Vec3f *pos, s16 n)
{
    VObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0x24;
        o->type = 0;
        o->update = VNumber_Update;
        Vec_Copy((f32 *)&o->pos, (f32 *)pos);
        *(s16 *)(o->buf + 0x10) = n;
    }
}

#include "player.h"
#include "lobby_state.h"
#include "lobby_npc.h"
/* The work area of menu 0x4F (gMenuWork). */
typedef struct Menu4F {
    /* 0x00 */ Em *npc;         /* the townsperson (MealScene_Open) */
    /* 0x04 */ u8 *pl;
    /* 0x08 */ s32 mask;
    /* 0x0C */ s16 a;
    /* 0x0E */ s16 b;
    /* 0x10 */ s16 timer;
    /* 0x12 */ s16 msg;
    /* 0x14 */ s8 state;
} Menu4F;
s16 Poogie_Lookup(s32 id);
void Talk_Say(s32 msg, s8 a);
void Npc_HoldRoute(Em *npc);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s8 gMenuInputLock;

void MealScene_Open(Em *npc)
{
    Menu4F *m = (Menu4F *)&gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = 0x4F;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gMenuInputLock = 6;
    m->npc = npc;
    m->pl = (u8 *)&gPlayers[gScene.localPlayer];
    m->msg = Poogie_Lookup(((NpcWork *)&npc->work)->id);
    Talk_Say(m->msg, 0);
    Npc_HoldRoute(npc);
    switch (gScene.area) {
    case 0xAD:
        m->a = 0;
        m->b = 1;
        break;
    case 0xAE:
        m->a = 3;
        m->b = 4;
        break;
    case 0xAF:
        m->a = 9;
        m->b = 10;
        break;
    }
    m->mask = 0;
    m->mask |= 1 << m->a;
    m->mask |= 1 << m->b;
    if (gLobby->mealEaten) {
        m->state = 10;
        Talk_Say(m->msg + 4, 0);
    }
}

void MealScene_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    CamReq_Cancel();
    CamFocus_End();
    if (gMenuWork != 0) {
        Npc_EnterRoutePoint((Em *)gMenuWork);
    }
}

#include "scene.h"
/* The effects of a meal: per facility level, three kinds. */
typedef struct MealFx {
    s16 a;
    s8 b;
    s8 c;
    s8 d;
    u8 pad;
} MealFx;
extern MealFx gMealEffects[][3];
extern char *gMealHealthTexts[];
extern char *gMealStaminaTexts[];
extern char *gMealAttackTexts[];
extern char *gMealDefenseTexts[];
extern char **gMealTasteTexts[];
u8 Notice_PushItem(char *);
u16 Rand_Next(s32 stream);
/* unit's own declaration: with the definition's u8 (s32 k) (sub_main/unk_3B4C80.c), MealScene_Update: 2 of 238 words differ */
s32 Village_FacilityLevel(s32 sub);
void Village_ClearNpcRec(u16 id);
s32 RateFlag_SetAndNotify(s32 a);
s32 Award_Grant(s32 a);

void Meal_ApplyEffects(u8 **arg)
{
    u8 *kind = *arg + 0x438;
    s8 row;
    s8 lvl;
    MealFx *e;
    char **msgs;

    switch (*kind) {
    case 1:
    case 5:
    case 11:
        row = 0;
        break;
    case 3:
    case 10:
    case 26:
        row = 1;
        break;
    default:
        row = 2;
        break;
    }
    lvl = Village_FacilityLevel(4);
    e = &gMealEffects[lvl][row];
    gScene.mealStamina = e->a;
    gScene.mealHealth = e->b;
    gScene.mealAttack = e->c;
    gScene.mealDefense = e->d;
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gMealStaminaTexts[0]);
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gMealHealthTexts[0]);
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gMealAttackTexts[0]);
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gMealDefenseTexts[0]);
    }
    gLobby->mealEaten = 1;
    Village_ClearNpcRec(*kind);
    if (gSave.mealPoints < 0x136) {
        gSave.mealPoints += lvl * 12;
    }
    if (gSave.recordCounts[32] < 0xFDE8) {
        gSave.recordCounts[32] += 2;
        if (gSave.recordCounts[32] >= 100) {
            if (!(gSave.rateFlags & 0x400)) {
                RateFlag_SetAndNotify(10);
            }
            Award_Grant(0x21);
        }
    }
    msgs = gMealTasteTexts[0];
    Notice_PushItem(msgs[(u16)Rand_Next(1) % 4]);
}

s32 Talk_Update(void);
void SeqA_Start(s32 a);
u8 SeqA_GetState(void);
void Actor_SetState(void *, s32, s32);
void Player_MoveToSpawn(Player *p, s16 id);
void Tutorial_Start(s32 a, s32 mask, u8 level); /* unit's own declaration: with the definition's void (s8 arg0, s32 mask, s32 level) (lobby/unk_78E4D0.c), MealScene_Update 2 of 238 words differ */
s32 Tutorial_IsDone(void);
void Npc_HideAndAdvance(Em *npc);
void CamReq_Kitchen(s32 a);
u16 LobbyPad_Get(s32 a);

void MealScene_Update(void)
{
    Menu4F *m = (Menu4F *)&gMenuWork;
    s32 k;

    LobbyPad_Get(0);
    switch (m->state) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            m->state++;
            Talk_Say(m->msg + 1, 0);
            break;
        case 3:
            m->state = 10;
            Talk_Say(m->msg + 2, 0);
            break;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state++;
            SeqA_Start(1);
            break;
        }
        break;
    case 2:
        if (SeqA_GetState() != 1) {
            m->state++;
            m->timer = 3;
            CamFocus_End();
            Player_MoveToSpawn((Player *)m->pl, m->a);
            Player_MoveToSpawn((Player *)m->npc, m->b);
            Actor_SetState(m->pl, 0, 0x25);
            Actor_SetState(m->npc, 0, 0x25);
        }
        break;
    case 3:
        if (--m->timer <= 0) {
            m->state++;
            SeqA_Start(2);
            switch (gScene.area) {
            case 0xAD:
            default:
                k = 0;
                break;
            case 0xAE:
                k = 1;
                break;
            case 0xAF:
                k = 2;
                break;
            case 0x57:
                k = 3;
                break;
            }
            CamReq_Kitchen(k);
            Tutorial_Start(2, m->mask, Village_FacilityLevel(4));
        }
        break;
    case 4:
        if (Tutorial_IsDone() == 1) {
            m->state++;
            Actor_SetState(m->pl, 0, 0x29);
            Actor_SetState(m->npc, 0, 0x29);
        }
        break;
    case 5:
        if (*(u16 *)(m->pl + 0x29C) == 0x261) {
            m->state++;
            Actor_SetState(m->pl, 0, 0x18);
            Actor_SetState(m->npc, 0, 0x18);
            Meal_ApplyEffects((u8 **)m);
        }
        break;
    case 6:
        if (*(s32 *)(m->pl + 0x154) <= 0) {
            m->state++;
        }
        break;
    case 7:
        if (*(s32 *)(m->pl + 0x154) <= 0) {
            m->state++;
            Talk_Say(m->msg + 3, 0);
        }
        break;
    case 8:
        switch (Talk_Update()) {
        case 0:
            m->state++;
            SeqA_Start(1);
            break;
        }
        break;
    case 9:
        if (SeqA_GetState() != 1) {
            Npc_HideAndAdvance(m->npc);
            m->npc = NULL;
            SeqA_Start(2);
            MealScene_Close();
        }
        break;
    case 10:
        switch (Talk_Update()) {
        case 0:
        case 3:
            MealScene_Close();
            break;
        }
        break;
    }
}

#include "player.h"
#include "scene.h"
typedef struct SpawnPoint {
    /* 0x00 */ u16 id;
    /* 0x02 */ u16 kind;
    /* 0x04 */ Vec3f pos;
    /* 0x10 */ u8 unk10[4];
    /* 0x14 */ u16 angle;
    /* 0x16 */ u8 unk16[2];
} SpawnPoint;
extern SpawnPoint *gAreaSpawnLists[];
u8 Area_GetSetIndex(u16 id);
void Player_MoveToSpawn(Player *p, s16 id)
{
    SpawnPoint *s = gAreaSpawnLists[(u8)Area_GetSetIndex(gScene.area)];

    while (s->kind != 0) {
        if (s->kind == 10 && s->id == id) {
            p->pos[0] = s->pos.x;
            p->pos[1] = s->pos.y;
            p->pos[2] = s->pos.z;
            p->rot[1] = s->angle;
            *(Vec3f *)p->moveTo = *(Vec3f *)p->pos;
            *(s16 *)((u8 *)p + 0xE) = p->rot[1];
            return;
        }
        s++;
    }
}

void VMuzzle_Update(VObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        VMuzzle_Init(o);
        break;
    case 1:
        VMuzzle_Run(o);
        break;
    case 2:
        VMuzzle_End(o);
        break;
    case 3:
        VMuzzle_Free(o);
        break;
    }
}

void VMuzzle_Draw(DrawSlot *slot);
void VMuzzle_Init(VObj *o)
{
    s32 *d = (s32 *)o->buf;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->timer = 0;
    o->state++;
    o->visible = 1;
    o->draw = 0;
    switch (o->type) {
    case 0:
        d[0] = 0;
        ((s16 *)d)[2] = 8;
        break;
    case 1:
        d[0] = Rand_Next(1);
        ((s16 *)d)[2] = 4;
        break;
    case 2:
        d[0] = Rand_Next(1);
        ((s16 *)d)[2] = 0x16;
        break;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = VMuzzle_Draw;
    } else {
        StageObj_Free(o);
    }
}

void VMuzzle_Run(VObj *o)
{
    u16 *d = (u16 *)o->buf;

    if (++o->timer > d[2]) {
        o->state++;
    } else if (o->node != NULL) {
        Vec_Copy(o->node->pos, (f32 *)&o->pos);
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
    }
}

void VMuzzle_End(VObj *o)
{
    o->state++;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void VMuzzle_Free(VObj *o)
{
    StageObj_Free(o);
}

/* unk_6B9160's .data (0x7DCDF0-0x7DD030) */
u8 gVFxAlphaKeys0[32] = {
#include "lobby/007DCDF0.inc"
};
u8 gVFxScaleKeys0[48] = {
#include "lobby/007DCE10.inc"
};
u8 gVFxAlphaKeys1[32] = {
#include "lobby/007DCE40.inc"
};
u8 gVFxScaleKeys1[64] = {
#include "lobby/007DCE60.inc"
};
u8 gVPartAlphaKeys[32] = {
#include "lobby/007DCEA0.inc"
};
u8 gVPartScaleKeys0[64] = {
#include "lobby/007DCEC0.inc"
};
u8 gVPartColorKeys0[32] = {
#include "lobby/007DCF00.inc"
};
u8 gVPartScaleKeys1[80] = {
#include "lobby/007DCF20.inc"
};
u8 gVPartColorKeys1[32] = {
#include "lobby/007DCF70.inc"
};
u8 gVPartScaleKeys2[64] = {
#include "lobby/007DCF90.inc"
};
u8 gVPartColorKeys2[32] = {
#include "lobby/007DCFD0.inc"
};
MealFx gMealEffects[3][3] = {
#include "lobby/007DCFF0.inc"
};
