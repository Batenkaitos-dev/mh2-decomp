/*
 * Game overlay code at 0x6CADF0-0x6D29F0: the EffEm08_* effect objects.
 * Built at optimization level 4: EffEm08_Run and EffEm08_DrawParts match only
 * there, every other function at both levels.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "objhead.h"
#include "stageobj.h"
#include "scene.h"
#include "sprite.h"
#include "capsule.h"

#pragma optimization_level 4

void Eff09_CreateOwned(Player *owner, f32 *pos, s32 kind);
void CamShake_SetA(s32 kind, f32 *pos);
/* unit's own declaration: with the definition's void (f32 *pos, s32 v, s16 k, f32 scale) (game/unk_874B90.c), EffEm08_Impact: 6 of 515 words differ */
void StageObj_K11_CreateAtScaled(f32 *pos, u16 r, s32 kind, f32 scale);
void StageObj_K11_CreateAt(f32 *pos, s16 k, u8 c, f32 scale);
/* unit's own declaration: with the definition's void (f32 *pos, s16 k, u8 c, s32 v, f32 scale) (game/unk_874B90.c), EffEm08_Impact: 3 of 515 words differ, EffEm08_Run: 4 of 2976 words differ */
void StageObj_K11_CreateAtYaw(f32 *pos, s32 kind, u8 type, u16 ang, f32 scale);
void StageObj_K12_CreateAtScaled(f32 *pos, s32 k, u8 c, f32 scale);
void StageObj_K0E_CreateScaledB(f32 *pos, s16 k, f32 scale);
void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f);
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
extern f32 EffEm08_K12Scales[];
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
typedef struct { s32 frame; u32 color; } ColorKey;
void Mtx_RotateZ(f32 *m, f32 a);
void EffEm08_CopyRespawnPos(u8 *a, u8 *b);
void EmFxSpark_SpawnAt(void *src, u8 k, u8 c, f32 f);
void Vec_Add(f32 *, f32 *, f32 *);
extern f32 (*EffEm08_RespawnTbls[])[2];
void Vec_RotateSelf(f32 *v, f32 *m);
extern f32 EffEm08_BoneOfs[];
extern f32 D_0089DA54[];
extern f32 D_0089DA58[];
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_RotateY(f32 *v, f32 angle);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Gfx_SetBlend(s32 a, s32 b);
extern f32 gCamMatrix[];
void StageObj_K12_CreateAt(f32 *pos, s32 k, u8 c);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
f32 Math_Absf(f32 x);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
u16 Rand_Next(s32 stream);
void EffEm08_Setup(ObjAttach *o);
void EffEm08_Run(ObjAttach *o);
void EffEm08_Hit(ObjAttach *);
void EffEm08_End(ObjAttach *o);
void EffEm08_Free(ObjAttach *o);
s32 Gs_SetState(u8 reg, u32 v);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *out, f32 *in);
/* unit's own declaration: with the definition's void (u16 flags) (sub_main/unk_3240E0.c), EffEm08_DrawModel: 1 of 39 words differ */
void Draw_SetBlendFlags(s32);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32);
struct MoveObj;
void MoveObj_Integrate(struct MoveObj *);
void EffEm08_Update(ObjAttach *o);
void Vec_Normalize(f32 *);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *, f32 *, f32 *);
void Vec_Sub(f32 *, f32 *, f32 *);
extern f32 gCamPos;
ObjAttach *ObjAttach_Alloc(s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Em_BonePos(void *obj, s16 bone, f32 *out);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern s16 EffEm08_MotionBase[];
extern void *EffEm08_MotionDesc;
void EffEm08_InitByKind(ObjAttach *, s32);
void EffEm08_Impact(ObjAttach *o, s16 n, f32 *pos);

void EffEm08_Init(ObjAttach *o, ObjHead *owner, u8 type, u8 var1)
{
    o->kind = 8;
    o->type = type;
    o->stateVar1 = var1;
    o->update = EffEm08_Update;
    o->owner = owner;
    o->isEm = owner->isEm;
    o->idx = owner->index;
    o->side = owner->side;
    o->yaw = owner->rot[1];
}

void EffEm08_CreateAtBone(Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            EffEm08_Init(o, (ObjHead *) em, a, b);
            Em_BonePos(em, bone, o->pos);
            o->ownerMotion = bone;
            o->aux = em->type;
            o->rotX = (u16) c;
            o->rotY = (u16) d + em->rotY;
            o->step = 0;
            o->stateVar0 = 0;
        }
    }
}

void EffEm08_CreateAtBone2(Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d, s32 e)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            EffEm08_Init(o, (ObjHead *) em, a, b);
            Em_BonePos(em, bone, o->pos);
            o->ownerMotion = bone;
            o->aux = em->type;
            o->rotX = (u16) c;
            o->rotY = (u16) d + em->rotY;
            o->step = 0;
            o->stateVar0 = e;
        }
    }
}

void EffEm08_CreateAt(void *owner, s32 arg1, s32 arg2, ObjAttach *parent, f32 *src)
{
    ObjAttach *o;

    if (parent->area == gScene.area) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            EffEm08_Init(o, owner, arg1 & 0xFF, arg2 & 0xFF);
            o->yaw = parent->yaw;
            Vec_Copy(o->pos, src);
            o->aux = parent->aux;
            o->side = parent->side;
            o->rotX = 0;
            o->rotY = parent->rotY;
            o->step = 1;
            o->stateVar0 = 0;
        }
    }
}

void EffEm08_CreateAt2(void *owner, s32 arg1, s32 arg2, ObjAttach *parent, f32 *src, s32 arg5, u16 rotY)
{
    ObjAttach *o;

    if (parent->area == gScene.area) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            EffEm08_Init(o, owner, arg1 & 0xFF, arg2 & 0xFF);
            o->yaw = rotY;
            Vec_Copy(o->pos, src);
            o->aux = parent->aux;
            o->side = parent->side;
            o->rotX = arg5 & 0xFFFF;
            o->rotY = rotY & 0xFFFF;
            o->step = 1;
            o->stateVar0 = 0;
        }
    }
}

void EffEm08_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffEm08_Setup(o);
        break;
    case 1:
        EffEm08_Run(o);
        break;
    case 2:
        EffEm08_End(o);
        break;
    case 3:
        EffEm08_Hit(o);
        break;
    case 4:
        EffEm08_End(o);
        break;
    case 5:
        EffEm08_End(o);
        break;
    case 9:
        EffEm08_End(o);
        break;
    case 7:
        EffEm08_End(o);
        break;
    case 6:
        EffEm08_Free(o);
        
    default:
        break;
    }
}

void EffEm08_InitByKind(ObjAttach *o, s32 owner)
{
    s16 n;
    u8 kind = o->type;

    switch (kind) {
    case 12:
    case 24:
        n = 0;
        break;
    default:
        n = o->stateVar1;
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, owner, n + EffEm08_MotionBase[kind], &EffEm08_MotionDesc);
}

/* EffEm08's object buffer (o->buf): a header, then one Em08Part per particle (EffEm08_PartCounts[type]). */
typedef struct Em08Part {
    s8 idx;                     /* 0x00 */
    u8 unk01;
    s16 unk02;                  /* 0x02 */
    f32 scale[3];               /* 0x04 */
    u16 rot[3];                 /* 0x10 */
    s16 spin[3];                /* 0x16 */
    u8 unk1C[3];                /* 0x1C */
    u8 unk1F;
} Em08Part;

typedef struct Em08Buf {
    u8 unk00;
    s8 unk01;
    u8 unk02;
    s8 unk03;
    s16 slot;                   /* 0x04: the second draw slot */
    u8 pad06[2];
    DrawSlot *ds;               /* 0x08 */
    f32 pos[3];                 /* 0x0C */
    f32 unk18[3];               /* 0x18 */
    u8 pad24[0x30 - 0x24];
    s16 bone;                   /* 0x30 */
    u16 unk32;
    f32 floorY;                 /* 0x34 */
    s16 unk38;
    u8 unk3A;
    s8 unk3B;
    Em08Part part[1];           /* 0x3C */
} Em08Buf;

extern f32 EffEm08_Val050ByType[];
extern f32 EffEm08_Val058ByType[];
extern s16 EffEm08_PartCounts[];
extern f32 EffEm08_StartPos[];
extern s16 EffEm08_SpinX[];
extern s16 EffEm08_SpinY[];
extern s16 EffEm08_BoneIdx[];
extern HitShape *EffEm08_Vals088[];
EmBone *Em_Bone(void *obj, s16 bone);
f32 AreaGrid2_GetFloorY(f32 *pos);
void EffEm08_DrawParts(DrawSlot *ds);
void EffEm08_AttachToBone(ObjAttach *o, u8 *b);
void EffEm08_Respawn(ObjAttach *o, u8 *b);

/* EffEm08's first frame (state 0): places the effect at its owner's bone (unless unk005 is 1), sets
 * its velocity from rotX/rotY, takes a draw slot for itself (EffEm08_DrawParts, part 0), resets the
 * particles, then per type: a second draw slot (part 1) and the particles' spins, scales and
 * random angles; the sound of types 6, 22 and 24; then the motion and EffEm08_Run. */
void EffEm08_Setup(ObjAttach *o)
{
    f32 m[16];
    f32 v[3];
    Em08Buf *b;
    Em08Part *p;
    Em *em;
    s32 n;
    s16 i;

    em = &gEms[o->idx];
    b = (Em08Buf *) o->buf;
    o->state = 1;
    p = b->part;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x200);
    o->wasHit = 0;
    o->area = em->area;
    b->unk00 = 0;
    b->unk01 = 0;
    b->unk02 = 0;
    b->unk03 = 0;
    b->bone = o->ownerMotion;
    b->unk32 = 0;
    o->ownerMotion = 0;
    o->velX = 0.0f;
    o->velY = 0.0f;
    o->velZ = EffEm08_Val050ByType[o->type];
    if (o->step != 1) {
        Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, b->bone)));
        v[0] = EffEm08_BoneOfs[o->type * 3];
        v[1] = D_0089DA54[o->type * 3];
        v[2] = D_0089DA58[o->type * 3];
        Vec_RotateSelf(v, m);
        Vec_Add(o->pos, o->pos, v);
        Vec_Copy(o->pos2, o->pos);
    }
    Vec_RotateX(&o->velX, 6.2831855f * o->rotX / 65536.0f);
    Vec_RotateY(&o->velX, 6.2831855f * o->rotY / 65536.0f);
    o->homingTurn = 0;
    o->gravity = EffEm08_Val058ByType[o->type];
    o->homingSpeed = 0;
    o->step = 0;
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->part = 0;
        o->drawSlotBuf->draw = EffEm08_DrawParts;
    } else {
        o->drawSlotBuf = NULL;
    }
    b->ds = NULL;
    n = EffEm08_PartCounts[o->type];
    for (i = 0; i < n; i++) {
        p->idx = i;
        p->unk02 = 0;
        p->rot[0] = 0;
        p->rot[1] = 0;
        p->rot[2] = 0;
        p->spin[0] = 0;
        p->spin[1] = 0;
        p->spin[2] = 0;
        p->unk1C[0] = 0;
        p->unk1C[1] = 0;
        p->unk1C[2] = 0;
        p->unk1F = 0;
        p->scale[2] = 1.0f;
        p->scale[1] = 1.0f;
        p->scale[0] = 1.0f;
        p++;
    }
    p = b->part;
    switch (o->type) {
    case 12:
        Vec_Copy(v, &EffEm08_StartPos[o->stateVar1 * 3]);
        Vec_RotateY(v, 6.2831855f * o->rotY / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        Vec_Copy(o->pos2, o->pos);
    case 0:
    case 11:
        b->slot = DrawSlot_Alloc();
        if (b->slot != -1) {
            b->ds = DrawSlot_Get(b->slot);
            b->ds->owner = o;
            b->ds->part = 1;
            b->ds->draw = EffEm08_DrawParts;
        } else {
            b->ds = NULL;
        }
        for (i = 0; i < n; i++, p++) {
            if (i < 3) {
                if (i == 0) {
                    p->rot[2] = Rand_Next(1);
                } else {
                    p->spin[0] = EffEm08_SpinX[i - 1] + (u16) (s32) (0.5f + 65536.0f * (0.1f * ((Rand_Next(1) & 0x1F) - 0x10)) / 360.0f);
                    p->spin[1] = EffEm08_SpinY[i - 1] + (u16) (s32) (0.5f + 65536.0f * (0.1f * ((Rand_Next(1) & 0x1F) - 0x10)) / 360.0f);
                }
            } else {
                p->rot[0] = 0xC000;
            }
        }
        break;
    case 1:
    case 18:
        b->unk01 = o->rotX >> 8;
        b->unk3B = (o->rotY - em->rotY) >> 8;
        b->unk38 = *(u16 *) ((u8 *) o->owner + 0x29C);
        EffEm08_AttachToBone(o, (u8 *) b);
        p->spin[2] = 0x71C;
        break;
    case 22:
        Em_PlaySeChecked(o->owner, 0x1C, 0, o->pos, 1, 0);
        goto grow;
    case 24:
        o->hitScale = EffEm08_K12Scales[o->stateVar1];
        switch (o->stateVar1) {
        default:
        case 0:
            Em_PlaySeChecked(o->owner, 0x18, 0, o->pos, 1, 0);
            break;
        case 1:
            Em_PlaySeChecked(o->owner, 0x27, 0, o->pos, 1, 0);
            break;
        }
    case 2:
    case 3:
    case 9:
    case 13:
    case 23:
    grow:
        if ((o->type == 9 && o->stateVar1 >= 2) || (o->type == 23 && o->stateVar1 > 0)) {
            o->velX *= 2.0f;
            o->velY *= 2.0f;
            o->velZ *= 2.0f;
        }
        b->slot = DrawSlot_Alloc();
        if (b->slot != -1) {
            b->ds = DrawSlot_Get(b->slot);
            b->ds->owner = o;
            b->ds->part = 1;
            b->ds->draw = EffEm08_DrawParts;
        } else {
            b->ds = NULL;
        }
        for (i = 0; i < n; i++) {
            p->idx = 0;
            p->unk02 = -i * 4;
            p->rot[2] = Rand_Next(1);
            p->spin[2] = 0x199A;
            p++;
        }
        break;
    case 4:
    case 21:
        b->unk01 = o->rotX >> 8;
        b->unk3B = (o->rotY - em->rotY) >> 8;
        b->unk38 = *(u16 *) ((u8 *) o->owner + 0x29C);
        EffEm08_AttachToBone(o, (u8 *) b);
        b->unk3A = o->stateVar0;
        o->stateVar0 = 0;
        for (i = 0; i < n; i++) {
            switch (p->idx) {
            case 0:
                p->spin[2] = 0x1000;
            }
            p++;
        }
        break;
    case 6:
        o->pos[1] = em->floorY;
        Em_PlaySeChecked(o->owner, 0x37, 0, o->pos, 1, 0);
        b->slot = DrawSlot_Alloc();
        if (b->slot != -1) {
            b->ds = DrawSlot_Get(b->slot);
            b->ds->owner = o;
            b->ds->part = 1;
            b->ds->draw = EffEm08_DrawParts;
        } else {
            b->ds = NULL;
        }
        for (i = 0; i < n; i++) {
            p->idx = i;
            if (i > 0) {
                if (i == 5) {
                    p->idx = 4;
                    p->unk02 = -5;
                }
                p->rot[2] = Rand_Next(1);
            }
            p++;
        }
        break;
    case 8:
        b->unk00 = EffEm08_BoneIdx[o->stateVar1 % 6] + 1;
        Vec_Copy(b->pos, (f32 *) ((u8 *) o->owner + 0xAC));
        p->unk02 = -7;
        break;
    case 10:
        o->pos[1] = AreaGrid2_GetFloorY(o->pos);
        break;
    case 14:
        for (i = 0; i < n; i++) {
            p->scale[2] = 1.0f;
            p->scale[1] = 1.0f;
            p->scale[0] = 1.0f;
            p++;
        }
        break;
    case 15:
        b->slot = DrawSlot_Alloc();
        if (b->slot != -1) {
            b->ds = DrawSlot_Get(b->slot);
            b->ds->owner = o;
            b->ds->part = 1;
            b->ds->draw = EffEm08_DrawParts;
        } else {
            b->ds = NULL;
        }
        p->scale[2] = 3.0f;
        p->scale[1] = 3.0f;
        p->scale[0] = 3.0f;
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        b->floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        break;
    case 16:
        p->scale[2] = 2.0f;
        p->scale[1] = 2.0f;
        p->scale[0] = 2.0f;
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        p->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    case 19:
        p->scale[2] = 1.5f;
        p->scale[1] = 1.5f;
        p->scale[0] = 1.5f;
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        p->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    case 20:
        p->scale[2] = 5.0f;
        p->scale[1] = 5.0f;
        p->scale[0] = 5.0f;
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        p->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        p->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    }
    if (o->type == 8) {
        EffEm08_Respawn(o, (u8 *) b);
    } else {
        EffEm08_InitByKind(o, (s32) em);
    }
    o->hitList = EffEm08_Vals088[o->hitListNo];
    o->hurtList = 0;
    EffEm08_Run(o);
}

extern s16 EffEm08_Limits[];
extern s16 EffEm08_Lives[];
extern s16 EffEm08_LimitsByIdx[];
void EffEm08_LerpPos(ObjAttach *o, ObjAttach *ref, f32 *out);
void EffEm08_PickDirection(ObjAttach *o, Em08Part *p);
extern s16 EffEm08_KeyIdx[];
extern s16 EffEm08_Steps[];
extern u32 EffEm08_Curves[];
extern f32 gCamDir[3];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void CamShake_SetAEverywhere(s32);
void StageObj_K11_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void StageObj_K11_CreateType3(f32 *pos, u16 v, u16 w, u8 a, u8 b);
void StageObj_K1C_CreateAtAngled(f32 *pos, u16 w, u16 v, u8 k, f32 scale);
f32 AreaGrid2_GetFloorY(f32 *pos);
/* EffEm08's update (state 1): counts its life (o->unk07C against EffEm08_Lives[type], or the
 * owner-tied lengths of types 1, 4, 18 and 21), moves it per type (MoveObj_Integrate, the
 * owner-following phases of types 1/4/18/21 with their stage objects), steps every particle's
 * key animation (EffEm08_Curves curves through Key_LerpVec3, spins, scales), then per type checks
 * the floor (EffEm08_Impact, camera shake, sparks, stage objects) and queues its draw slots. */
/* EffEm08_Run needs optimization level 4 (the rest of the file matches at 3). */
void EffEm08_Run(ObjAttach *o)
{
    f32 v[3];
    f32 pos[3];
    f32 pos2[3];
    f32 d[3];
    Em *em;
    u8 spark;
    Em08Buf *b;
    Em08Part *p;
    Em08Part *top;
    Em08Part *q;
    Em *owner;
    f32 floorY;
    f32 dist;
    f32 f;
    s16 n;
    s16 life;
    s16 lim;
    s16 step;
    s16 key;
    s16 i;
    s16 t;
    u8 type;
    s32 x;
    u32 c;

    b = (Em08Buf *) o->buf;
    em = &gEms[o->idx];
    type = o->type;
    p = b->part;
    spark = 0;
    n = EffEm08_PartCounts[type];
    life = EffEm08_Lives[type];
    key = EffEm08_KeyIdx[type];
    step = EffEm08_Steps[type];
    lim = EffEm08_Limits[type];
    top = p;
    if (!((type == 8 || (type == 15 && o->hitPhase == 1)) && type == 16 && o->hitPhase == 1)) {
        o->hitFrames = 0x63;
    }
    o->yaw = o->rotY;
    switch (o->type) {
    case 0:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 19:
    case 20:
    case 22:
    case 23:
    case 24:
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case 1:
    case 18:
        life = o->stateVar0;
        break;
    case 4:
        life = b->unk3A + 31;
        break;
    case 8:
        break;
    case 21:
        life = b->unk3A + 31;
        break;
    }
    if (life < ++o->ownerMotion) {
        o->hitFrames = 0;
        o->state = 2;
        return;
    }
    switch (o->type) {
    case 0:
    case 11:
    case 12:
        switch (o->type) {
        case 0:
            t = 30;
            break;
        case 11:
            t = 30;
            break;
        default:
            t = life;
            break;
        }
        if (t < o->ownerMotion) {
            f = 1.0f / (o->ownerMotion - t);
            o->velX *= f;
            o->velY *= f;
            o->velZ *= f;
        }
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area) {
            if (o->ownerMotion < life - 5) {
                if (o->ownerMotion == 1 || o->ownerMotion % 3 != 1) {
                    StageObj_K11_CreateType3(o->pos, o->rotX, o->rotY, 0, ((Em *) o->owner)->type);
                } else {
                    StageObj_K11_CreateType3(o->pos, o->rotX, o->rotY, 2, ((Em *) o->owner)->type);
                }
            }
            if (o->type == 12) {
                if (o->pos2[1] >= 3000.0f && o->pos[1] < 3000.0f) {
                    Em_PlaySeChecked((Em *) o->owner, 0x42, 0, o->pos, 7, 0);
                }
                if (o->ownerMotion % 5 == 0) {
                    Em_PlaySeChecked((Em *) o->owner, 0x3B, 0, o->pos, 0x18, 0);
                }
            } else if (o->ownerMotion % 5 == 0) {
                Em_PlaySeChecked((Em *) o->owner, 0x3A, 0, o->pos, 0x18, 0);
            }
        }
        break;
    case 1:
        owner = (Em *) o->owner;
        if (!owner->active || owner->taskPhase == 3 || b->unk38 != owner->motionId[0]) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        EffEm08_AttachToBone(o, (u8 *) b);
        switch (b->unk02) {
        case 0:
            lim = 10;
            if (++b->unk32 > lim) {
                b->unk02++;
                lim = life - 30;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p->unk02 = 0;
                    p++;
                }
                p = top;
            }
            break;
        case 1:
            lim = life - 30;
            if (++b->unk32 > lim) {
                lim = 20;
                b->unk02++;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p->unk02 = 0;
                    p++;
                }
                p = top;
            }
            break;
        default:
        case 2:
            lim = 20;
            if (++b->unk32 > lim) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->state = 2;
                return;
            }
            break;
        }
        o->yaw = o->rotY;
        break;
    case 2:
    case 3:
    case 13:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion >= 3 && (o->ownerMotion & 1)) {
            if (o->type == 2 || o->type == 3) {
                StageObj_K11_CreateAtYaw(o->pos, 0x12, ((Em *) o->owner)->type, o->rotY, 1.0f);
            } else if (o->type == 13) {
                StageObj_K11_CreateAtYaw(o->pos, 0x12, ((Em *) o->owner)->type, o->rotY, 2.0f);
            }
        }
        break;
    case 4:
        if (o->stateVar0 != 0xFF) {
            owner = (Em *) o->owner;
            if (!owner->active || owner->taskPhase == 3
                || (owner->motionId[0] != b->unk38 && owner->motionId[0] != 0x45A)) {
                o->stateVar0 = 0xFF;
            }
        }
        if (o->stateVar0 != 0xFF) {
            EffEm08_AttachToBone(o, (u8 *) b);
        }
        switch (b->unk02) {
        case 0:
            lim = 11;
            if (++b->unk32 > lim) {
                b->unk02++;
                if (o->stateVar0 != 0xFF && o->area == gScene.area) {
                    StageObj_K11_CreateAtBone(em, b->bone, 7, 0);
                }
                b->unk32 = 0;
                lim = 7;
                for (i = 0; i < n; i++) {
                    q = &p[i];
                    q->unk02 = 0;
                    switch (q->idx) {
                    case 0:
                        q->rot[2] = 0;
                        q->spin[2] = 0;
                        break;
                    case 1:
                        q->rot[2] = 0;
                        q->spin[2] = 0x1000;
                        break;
                    case 2:
                        q->rot[2] = 0;
                        q->spin[2] = 0x1000;
                        break;
                    }
                }
            }
            break;
        case 1:
            lim = 7;
            if (++b->unk32 > lim) {
                b->unk02++;
                b->unk32 = 0;
                lim = b->unk3A;
                for (i = 0; i < n; i++) {
                    p[i].unk02 = 0;
                }
            }
            break;
        case 2:
            lim = b->unk3A;
            if (++b->unk32 > lim) {
                lim = 13;
                b->unk02++;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p[i].unk02 = 0;
                }
                o->stateVar0 = 0xFF;
                o->hitPhase = 0;
                if (o->area == gScene.area) {
                    StageObj_K11_CreateAtYaw(o->pos2, 6, ((Em *) o->owner)->type, o->rotY, 1.0f);
                }
            }
            break;
        case 3:
            lim = 13;
            if (++b->unk32 > lim) {
                o->hitFrames = 0;
                o->state = 2;
                return;
            }
            break;
        default:
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
        switch (b->unk02) {
        case 0:
            step = 2;
            key = 5;
            break;
        case 1:
            step = 1;
            key = 6;
            break;
        case 2:
            step = 0;
            key = -1;
            break;
        case 3:
            step = 1;
            key = 8;
            break;
        }
        if (b->unk02 == 1 || b->unk02 == 2) {
            if (o->area == gScene.area && !(o->ownerMotion & 3)) {
                StageObj_K11_CreateAtYaw(o->pos2, 0, ((Em *) o->owner)->type, o->rotY, 1.0f);
            }
        }
        o->yaw = o->rotY;
        break;
    case 5:
    case 7:
    case 9:
    case 10:
    case 14:
    case 16:
    case 17:
    case 20:
    case 22:
    case 23:
    case 24:
        MoveObj_Integrate((struct MoveObj *)o);
        break;
    case 15:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->velY <= -300.0f) {
            o->velY = -300.0f;
        }
        break;
    case 6:
        if (o->ownerMotion >= life - 2) {
            if (o->ownerMotion == life - 2) {
                o->hitPhase = 0;
                spark = 1;
            } else {
                return;
            }
        }
        MoveObj_Integrate((struct MoveObj *)o);
        break;
    case 8:
        owner = (Em *) o->owner;
        if (!owner->active || owner->taskPhase == 3) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        if (owner->stateGroup == 4 || owner->stateGroup == 5) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case 18:
        owner = (Em *) o->owner;
        if (!owner->active || owner->taskPhase == 3 || b->unk38 != owner->motionId[0]) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        EffEm08_AttachToBone(o, (u8 *) b);
        switch (b->unk02) {
        case 0:
            lim = 10;
            if (++b->unk32 > lim) {
                lim = 2;
                b->unk02++;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p->unk02 = 0;
                    p++;
                }
                p = top;
            }
            break;
        case 1:
            lim = 2;
            if (++b->unk32 > life - 30) {
                lim = 20;
                b->unk02++;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p->unk02 = 0;
                    p++;
                }
                p = top;
            }
            break;
        default:
        case 2:
            lim = 20;
            if (++b->unk32 > lim) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->state = 2;
                return;
            }
            break;
        }
        o->yaw = o->rotY;
        break;
    case 19:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && !(o->ownerMotion & 1)) {
            StageObj_K1C_CreateAt(o->pos, 0x16, 2.0f);
        }
        break;
    case 21:
        if (o->stateVar0 != 0xFF) {
            owner = (Em *) o->owner;
            if (!owner->active || owner->taskPhase == 3
                || (owner->motionId[0] != b->unk38 && owner->motionId[0] != 0x439)) {
                o->stateVar0 = 0xFF;
            }
        }
        if (o->stateVar0 != 0xFF) {
            EffEm08_AttachToBone(o, (u8 *) b);
        }
        switch (b->unk02) {
        case 0:
            lim = 11;
            if (++b->unk32 > lim) {
                b->unk02++;
                b->unk32 = 0;
                lim = 7;
                for (i = 0; i < n; i++) {
                    q = &p[i];
                    q->unk02 = 0;
                    switch (q->idx) {
                    case 0:
                        q->rot[2] = 0;
                        q->spin[2] = 0;
                        break;
                    case 1:
                        q->rot[2] = 0;
                        q->spin[2] = 0x1000;
                        break;
                    case 2:
                        q->rot[2] = 0;
                        q->spin[2] = 0x1000;
                        break;
                    }
                }
            }
            break;
        case 1:
            lim = 7;
            if (++b->unk32 > lim) {
                b->unk02++;
                b->unk32 = 0;
                lim = b->unk3A;
                for (i = 0; i < n; i++) {
                    p[i].unk02 = 0;
                }
            }
            break;
        case 2:
            lim = b->unk3A;
            if (++b->unk32 > lim) {
                lim = 13;
                b->unk02++;
                b->unk32 = 0;
                for (i = 0; i < n; i++) {
                    p[i].unk02 = 0;
                }
                o->stateVar0 = 0xFF;
                o->hitPhase = 0;
            }
            break;
        case 3:
            lim = 13;
            if (++b->unk32 > lim) {
                o->hitFrames = 0;
                o->state = 2;
                return;
            }
            break;
        default:
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
            return;
        }
        x = b->unk02;
        switch (x) {
        case 0:
            step = 2;
            key = 5;
            break;
        case 1:
            step = 1;
            key = 6;
            break;
        case 2:
            step = 0;
            key = -1;
            break;
        case 3:
            step = 1;
            key = 8;
            break;
        }
        if ((x == 1 || x == 2 || x == 3) && o->area == gScene.area && !(o->ownerMotion & 1)) {
            if (o->stateVar1 == 1) {
                f = 0.7f;
            } else {
                f = 1.0f;
            }
            if (x == 3) {
                f *= 1.0f - b->unk32 / (f32) lim;
            }
            if (f > 0.0f) {
                StageObj_K1C_CreateAtAngled(o->pos2, o->rotX, o->rotY, 0x24, f);
            }
        }
        o->yaw = o->rotY;
        break;
    }
    for (i = 0; i < n; i++, p++) {
        switch (o->type) {
        case 0:
            if (i < 3) {
                lim = 36;
                if (p->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        case 6:
            lim = EffEm08_LimitsByIdx[p->idx];
            break;
        case 8:
            if (p->unk02 == 0) {
                if (b->unk00 == 0) {
                    p->unk02 = -80;
                    o->hitPhase = 0;
                } else {
                    EffEm08_Respawn(o, (u8 *) b);
                }
            }
            break;
        case 11:
            if (i < 3) {
                lim = 36;
                if (p->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        case 12:
            if (i < 3) {
                lim = 1000;
                if (p->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        }
        if (++p->unk02 <= 0) {
            key += step;
            continue;
        }
        if (p->unk02 > lim) {
            switch (o->type) {
            case 2:
            case 3:
            case 13:
            case 18:
                p->unk02 = 1;
                break;
            case 6:
                EffEm08_PickDirection(o, p);
                if (p->unk02 <= 0) {
                    key += step;
                    continue;
                }
                break;
            case 7:
                break;
            case 8:
                p->unk02 = 0;
                key += step;
                continue;
            case 12:
                if (i < 3) {
                    p->unk02 = 0;
                    break;
                }
                key += step;
                continue;
            default:
                key += step;
                continue;
            }
        }
        switch (o->type) {
        case 0:
        case 11:
        case 12:
            c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
            if (p->idx == 0) {
                if (!(o->ownerMotion & 1)) {
                    o->stateVar0++;
                    o->stateVar0 &= 7;
                }
            } else {
                p->rot[0] += p->spin[0];
                p->rot[1] += p->spin[1];
                if (p->idx == 1) {
                    p->scale[0] *= 0.9f;
                    p->scale[1] *= 0.9f;
                    p->scale[2] *= 0.9f;
                }
            }
            switch (o->type) {
            case 11:
                p->scale[0] *= 1.5f;
                p->scale[1] *= 1.5f;
                p->scale[2] *= 1.5f;
                break;
            }
            break;
        case 1:
            switch (b->unk02) {
            case 0:
                c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
                break;
            case 1:
                break;
            case 2:
                p->scale[0] = 1.0f - p->unk02 / (f32) lim;
                break;
            }
            p->rot[2] += p->spin[2];
            break;
        case 2:
        case 3:
        case 9:
        case 13:
        case 22:
        case 23:
        case 24:
            p->rot[2] += p->spin[2];
            Key_LerpVec3((void *) EffEm08_Curves[key], p->scale, p->unk02);
            if (o->type == 13) {
                p->scale[0] *= 2.0f;
                p->scale[1] *= 2.0f;
                p->scale[2] *= 2.0f;
            }
            break;
        case 4:
        case 21:
            switch (b->unk02) {
            case 0:
                switch (p->idx) {
                case 0:
                    p->unk1F = 1;
                    c = EffEm08_Curves[key++];
                    Key_LerpVec3((void *) c, p->scale, p->unk02);
                    break;
                default:
                    p->unk1F = 0;
                    continue;
                }
                break;
            case 1:
                p->unk1F = 1;
                p->unk01 = 0xFF;
                switch (p->idx) {
                case 0:
                case 1:
                    c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
                    break;
                case 2:
                    p->unk1F = 0;
                    p->scale[0] = 1.0f;
                    p->scale[1] = 1.0f;
                    p->scale[2] = 1.0f;
                    if (o->type == 21 && o->stateVar1 == 1) {
                        p->unk1F = 0;
                    }
                    break;
                }
                break;
            case 2:
                p->unk01 = 0xFF;
                switch (p->idx) {
                case 0:
                    p->unk1F = 0;
                    break;
                case 1:
                    p->unk1F = 1;
                    p->scale[0] = 0.8f + 0.4f / 1000.0f * (Rand_Next(1) & 0x3FF);
                    p->scale[1] = 1.0f;
                    p->scale[2] = 1.0f;
                    break;
                case 2:
                    p->unk1F = 1;
                    p->scale[0] = 1.0f;
                    p->scale[1] = 1.0f;
                    p->scale[2] = 1.0f;
                    if (o->type == 21 && o->stateVar1 == 1) {
                        p->unk1F = 0;
                    }
                    break;
                }
                break;
            case 3:
                p->unk01 = 0xFF;
                switch (p->idx) {
                case 0:
                    p->unk1F = 0;
                    break;
                case 1:
                    p->unk1F = 1;
                    c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
                    break;
                case 2:
                    p->unk1F = 1;
                    c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
                    if (o->type == 21 && o->stateVar1 == 1) {
                        p->unk1F = 0;
                    }
                    break;
                }
                break;
            }
            p->rot[2] += p->spin[2];
            break;
        case 6:
            c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
            p->scale[0] *= 1.5f;
            p->scale[1] *= 1.5f;
            p->scale[2] *= 1.5f;
            break;
        case 7:
            switch (p->idx) {
            case 0:
                p->scale[0] = 0.8f + 0.2f / 1000.0f * (Rand_Next(1) & 0x3FF);
                break;
            default:
            case 1:
                f = 0.8f + 0.2f / 1000.0f * (Rand_Next(1) & 0x3FF);
                p->scale[0] = f;
                p->scale[1] = f;
                break;
            }
            break;
        case 8:
            Key_LerpVec3((void *) EffEm08_Curves[key], p->scale, p->unk02);
            break;
        case 15:
        case 16:
        case 19:
        case 20:
            p->rot[0] += p->spin[0];
            p->rot[1] += p->spin[1];
            p->rot[2] += p->spin[2];
            break;
        case 18:
            switch (b->unk02) {
            case 0:
                p->scale[2] = 0.7f * (p->unk02 / (f32) lim);
                break;
            case 1:
                c = EffEm08_Curves[key++];
            Key_LerpVec3((void *) c, p->scale, p->unk02);
                p->scale[2] *= 0.7f;
                break;
            case 2:
                p->scale[0] = 1.0f - p->unk02 / (f32) lim;
                break;
            }
            p->rot[2] += p->spin[2];
            break;
        }
    }
    switch (o->type) {
    case 0:
    case 11:
    case 12:
        switch (o->type) {
        case 0:
            t = 33;
            break;
        case 11:
            t = 33;
            break;
        default:
            t = 36;
            break;
        }
        if (o->ownerMotion <= t) {
            floorY = AreaGrid2_GetFloorYFiltered(o->pos);
            if (o->pos[1] <= floorY) {
                if (o->type != 12 && floorY - o->pos[1] > 100.0f) {
                    EffEm08_Impact(o, 2, o->pos);
                } else {
                    pos[0] = o->pos[0];
                    pos[1] = floorY;
                    pos[2] = o->pos[2];
                    EffEm08_Impact(o, 0, pos);
                }
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
            }
            if (o->area == gScene.area) {
                Mtx_GetTrans(v, gCamMatrix);
                v[0] -= o->pos[0];
                v[1] -= o->pos[1];
                v[2] -= o->pos[2];
                if (0.0f < Vec_Dot(v, gCamDir)) {
                    dist = Vec_Dot(v, v);
                    if (10000.0f > dist) {
                        CamShake_SetAEverywhere(2);
                    }
                    if (90000.0f > dist) {
                        CamShake_SetAEverywhere(1);
                    } else if (360000.0f > dist) {
                        CamShake_SetAEverywhere(0);
                    }
                }
            }
        }
        break;
    case 1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 20.0f * top->scale[2];
        Vec_RotateX(v, 6.2831855f * o->rotX / 65536.0f);
        Vec_RotateY(v, 6.2831855f * o->rotY / 65536.0f);
        o->pos[0] = o->pos2[0] + v[0];
        o->pos[1] = o->pos2[1] + v[1];
        o->pos[2] = o->pos2[2] + v[2];
        switch (b->unk00) {
        case 0:
            floorY = AreaGrid2_GetFloorYFiltered(o->pos);
            if (o->pos[1] <= floorY) {
                b->unk00++;
                pos[0] = o->pos[0];
                if (floorY >= o->pos2[1]) {
                    pos[1] = o->pos[1];
                } else {
                    pos[1] = floorY;
                }
                pos[2] = o->pos[2];
                b->floorY = pos[1];
                if (o->stateVar1 == 1) {
                    EffEm08_LerpPos(o, (ObjAttach *) b, pos);
                    EffEm08_CreateAt(em, 10, 0, o, pos);
                }
            }
            break;
        case 1:
            if (o->stateVar1 != 1 && !(o->ownerMotion & 1)) {
                EffEm08_LerpPos(o, (ObjAttach *) b, pos);
                EffEm08_Impact(o, 0, pos);
            }
            if (b->unk02 > 2) {
                b->unk00++;
            }
            break;
        case 2:
            break;
        }
        break;
    case 2:
    case 3:
    case 9:
    case 13:
    case 16:
    case 17:
    case 19:
    case 20:
    case 22:
    case 23:
    case 24:
        floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floorY) {
            if (floorY - o->pos[1] > 100.0f) {
                EffEm08_Impact(o, 2, o->pos);
            } else {
                pos[0] = o->pos[0];
                pos[1] = floorY;
                pos[2] = o->pos[2];
                EffEm08_Impact(o, 0, pos);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 15:
        floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        if (floorY <= b->floorY - 500.0f) {
            pos2[0] = o->pos[0];
            pos2[1] = b->floorY;
            pos2[2] = o->pos[2];
            floorY = AreaGrid2_GetFloorYFiltered(pos2);
        }
        if (o->pos[1] <= floorY) {
            pos[0] = o->pos[0];
            pos[1] = floorY;
            pos[2] = o->pos[2];
            EffEm08_Impact(o, 0, pos);
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        b->floorY = floorY;
        break;
    case 4:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 6400.0f;
        Vec_RotateX(v, 6.2831855f * o->rotX / 65536.0f);
        Vec_RotateY(v, 6.2831855f * o->rotY / 65536.0f);
        o->pos[0] = o->pos2[0] + v[0];
        o->pos[1] = o->pos2[1] + v[1];
        o->pos[2] = o->pos2[2] + v[2];
        break;
    case 5:
        floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floorY) {
            if (floorY - o->pos[1] > 200.0f) {
                EffEm08_Impact(o, 2, o->pos);
            } else {
                pos[0] = o->pos[0];
                pos[1] = floorY;
                pos[2] = o->pos[2];
                EffEm08_Impact(o, 0, pos);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 6:
        floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        if (Math_Absf(floorY - o->pos[1]) > 100.0f) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        } else {
            o->pos[1] = floorY;
        }
        Vec_Sub(d, o->pos, o->pos2);
        o->rotX = (s32) (0.5f + 65536.0f * Math_Atan2(d[1], Math_Sqrtf(d[0] * d[0] + d[2] * d[2])) / 6.2831855f) & 0xFFFF;
        if (spark && o->area == gScene.area) {
            EmFxSpark_SpawnAt(o->pos, 5, ((Em *) o->owner)->type, 1.0f);
        }
        break;
    case 7:
        floorY = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floorY) {
            if (floorY - o->pos[1] > 100.0f) {
                EffEm08_Impact(o, 2, o->pos);
            } else {
                pos[0] = o->pos[0];
                pos[1] = floorY;
                pos[2] = o->pos[2];
                EffEm08_Impact(o, 0, pos);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 10:
        floorY = AreaGrid2_GetFloorY(o->pos);
        if (Math_Absf(floorY - o->pos[1]) > 100.0f) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        } else {
            o->pos[1] = floorY;
        }
        if (o->area == gScene.area && !(o->ownerMotion & 1)) {
            if (!(o->ownerMotion & 2)) {
                Em_PlaySeChecked((Em *) o->owner, 0x25, 0, o->pos, 1, 0);
            }
            StageObj_K11_CreateAt(o->pos, 13, ((Em *) o->owner)->type, 1.0f);
        }
        break;
    case 14:
        if (o->ownerMotion % 3 == 0) {
            floorY = AreaGrid2_GetFloorY(o->pos);
            pos[0] = o->pos[0];
            pos[1] = floorY;
            pos[2] = o->pos[2];
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 5, 1.0f);
            if (o->ownerMotion >= 4) {
                StageObj_K1C_CreateAtYaw(pos, o->rotY, (Rand_Next(1) & 1) + 7, 1.0f);
            }
            if (o->ownerMotion >= 8) {
                StageObj_K1C_CreateAt(pos, 6, 1.0f);
            }
        }
        break;
    case 18:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1300.0f * top->scale[2];
        Vec_RotateX(v, 6.2831855f * o->rotX / 65536.0f);
        Vec_RotateY(v, 6.2831855f * o->rotY / 65536.0f);
        o->pos[0] = o->pos2[0] + v[0];
        o->pos[1] = o->pos2[1] + v[1];
        o->pos[2] = o->pos2[2] + v[2];
        if (o->ownerMotion % 5 == 0) {
            floorY = AreaGrid2_GetFloorYFiltered(o->pos);
            if (o->pos[1] > floorY - 50.0f && o->pos[1] < 50.0f + floorY) {
                pos[0] = o->pos[0];
                pos[1] = floorY;
                pos[2] = o->pos[2];
                b->floorY = pos[1];
                EffEm08_Impact(o, 0, pos);
            } else if (o->pos[1] < floorY && o->pos2[1] > floorY) {
                f = (floorY - o->pos2[1]) / (o->pos[1] - o->pos2[1]);
                v[0] = f * (o->pos[0] - o->pos2[0]);
                v[2] = f * (o->pos[2] - o->pos2[2]);
                pos[0] = o->pos2[0] + v[0];
                pos[1] = floorY;
                pos[2] = o->pos2[2] + v[2];
                floorY = AreaGrid2_GetFloorYFiltered(pos);
                if (pos[1] > floorY - 50.0f || pos[1] < 50.0f + floorY) {
                    pos[1] = floorY;
                }
                EffEm08_Impact(o, 0, pos);
            }
        }
        break;
    case 21:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 6400.0f;
        Vec_RotateX(v, 6.2831855f * o->rotX / 65536.0f);
        Vec_RotateY(v, 6.2831855f * o->rotY / 65536.0f);
        o->pos[0] = o->pos2[0] + v[0];
        o->pos[1] = o->pos2[1] + v[1];
        o->pos[2] = o->pos2[2] + v[2];
        break;
    }
    if (o->drawSlotBuf != NULL) {
        switch (o->type) {
        case 1:
        case 4:
        case 18:
        case 21:
            Vec_Copy(o->drawSlotBuf->pos, o->pos2);
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40, 1);
            break;
        case 8:
            Vec_Copy(o->drawSlotBuf->pos, b->unk18);
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40, 1);
            break;
        case 10:
            return;
        default:
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40, 0);
            break;
        }
    }
    if (b->ds != NULL) {
        switch (o->type) {
        case 0:
        case 11:
        case 12:
        case 15:
            b->ds->pos[0] = o->pos[0];
            b->ds->pos[1] = 10.0f + floorY;
            b->ds->pos[2] = o->pos[2];
            OT_AddByDepth(gDrawOtSorted, b->ds, 0x40, 0);
            break;
        case 2:
        case 3:
        case 9:
        case 13:
        case 22:
        case 23:
        case 24:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -20.0f;
            Vec_RotateSelf(v, gCamMatrix);
            b->ds->pos[0] = o->pos[0] + v[0];
            b->ds->pos[1] = o->pos[1] + v[1];
            b->ds->pos[2] = o->pos[2] + v[2];
            OT_AddByDepth(gDrawOtSorted, b->ds, 0x40, 0);
            break;
        case 6:
            Vec_Copy(b->ds->pos, o->pos);
            OT_AddByDepth(gDrawOtSorted, b->ds, 0x40, 0);
            break;
        }
    }
}

void EffEm08_Hit(ObjAttach *o)
{
    f32 pos[3];

    switch (o->type) {
    case 0:
    case 5:
    case 7:
    case 11:
    case 12:
    case 19:
        pos[0] = o->pos[0];
        pos[1] = o->pos[1];
        pos[2] = o->pos[2];
        o->hitFrames = 0;
        o->state = 2;
        EffEm08_Impact(o, 1, pos);
        break;
    case 2:
    case 3:
    case 9:
    case 13:
    case 14:
    case 16:
    case 17:
    case 20:
    case 22:
    case 23:
    case 24:
        o->hitFrames = 0;
        o->state = 2;
        break;
    default:
        EffEm08_Run(o);
        break;
    }
}

void EffEm08_End(ObjAttach *o)
{
    u8 *w;

    w = o->buf;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free(o->drawSlot);
    }
    if (*(s32 *)(w + 8) != 0) {
        DrawSlot_Free(*(s16 *)(w + 4));
    }
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
}

void EffEm08_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm08_DrawModel(ModelMatSet *set, f32 *v, u32 color, s32 flags, ModelMat *mats)
{
    if ((set != NULL) && (set->id != -1)) {
        Gs_SetState(0x1A, (s32) v);
        Gs_SetState(0x67, (s32) color);
        Gs_SetLightList(mats, set);
        Draw_ApplyStates(set->flags);
        Draw_SetBlendFlags(flags);
        Model_Draw(set->id, 0);
        Draw_SetDefaultState();
    }
}

s16 EmSlot_FindKind(s16 id);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);
void Eff_ColorKeyLerp(ColorKey *k, s32 t, u32 *out);
void EffEm08_Billboard(f32 *arg0, void *arg1);
extern f32 gCamYawMtx[];
extern s16 EffEm08_Models[];
extern s16 EffEm08_Lives[];
extern ColorKey *EffEm08_KeysP12[];
extern ColorKey *EffEm08_KeysP12B[];
extern ColorKey EffEm08_Keys2[];
extern ColorKey EffEm08_Keys13[];
extern ColorKey EffEm08_Keys23[];
extern ColorKey EffEm08_KeysDef[];
extern ColorKey EffEm08_ColorKeysA[];
extern ColorKey EffEm08_ColorKeysB[];
extern ColorKey EffEm08_ColorKeysC[];
extern ColorKey EffEm08_ColorKeysD[];
extern s16 EffEm08_LimitsByIdx[];
extern ColorKey *EffEm08_ColorKeysByIdx[];
extern s16 EffEm08_KeysByIdx[];
extern s16 EffEm08_ModelIdx[];

/* Draws an Em08 effect slot ds: per effect type (o->type, 25 kinds) the part models of the
 * monster set (em) or the stage set with their scale, rotation, colour keys and texture scroll
 * (Gs_SetState 0x19). Needs optimization level 4, like EffEm08_Run. */
void EffEm08_DrawParts(DrawSlot *ds)
{
    u8 flat2;
    u8 flat;
    ObjAttach *o;
    Em08Buf *b;
    Em08Part *p;
    Model *em;
    Model *set;
    ModelMat *emMats;
    ModelMat *mats;
    ModelMatSet *mdl;
    ModelMat *mat;
    ColorKey **keysp;
    ColorKey *keys;
    s16 model;
    s32 mode;
    s16 k0;
    s16 cnt2;
    u8 alpha;
    u8 r;
    u8 g;
    u8 bl;
    u8 flag;
    s16 cnt;
    s16 i;
    s16 n;
    s16 k;
    s16 last;
    s32 div;
    s16 thr;
    s32 j;
    f32 f;
    f32 sc;
    f32 dy;
    f32 a;
    f32 ax;
    f32 ay;
    f32 az;
    u32 color;
    f32 m[16];
    f32 tm[16];
    f32 v[3];
    s16 lim;

    mode = 0;
    o = ds->owner;
    set = gStageModelSet;
    b = (Em08Buf *) o->buf;
    p = b->part;
    if (o->area != gScene.area) {
        goto end;
    }
    switch (o->type) {
    default:
        k = EmSlot_FindKind(o->aux);
        if (k == -1) {
            goto end;
        }
        em = gScene.partModels[k];
        if (em == NULL || !em->active) {
            goto end;
        }
        emMats = em->mats;
        break;
    case 9:
    case 16:
    case 20:
    case 22:
    case 23:
    case 24:
        break;
    }
    if (set == NULL || !set->active) {
        goto end;
    }
    mats = set->mats;
    Gs_SetState(0x60, 0);
    model = EffEm08_Models[o->type];
    switch (o->type) {
    case 0:
    case 11:
    case 12:
        last = EffEm08_Lives[o->type];
        if (o->type == 0) {
            div = 6;
            thr = 30;
            cnt = 1;
            r = 0xFF;
            n = 3;
            g = 0xCB;
            keysp = EffEm08_KeysP12;
            bl = 0x79;
        } else if (o->type == 11) {
            div = 6;
            thr = 30;
            cnt = 1;
            r = 0xFF;
            n = 3;
            g = 0xF3;
            keysp = EffEm08_KeysP12B;
            bl = 0xDF;
        } else {
            div = 1000;
            thr = 1000;
            cnt = 1;
            r = 0xFF;
            n = 3;
            g = 0xF3;
            keysp = EffEm08_KeysP12B;
            bl = 0xDF;
        }
        if (o->ownerMotion < thr) {
            alpha = 0xFF;
        } else {
            f = (1.0f / div) * (last - o->ownerMotion);
            alpha = 255.0f * f;
        }
        switch (ds->part) {
        case 0:
            Gs_SetState(0x6C, 0);
            for (i = 0; i < n; i++) {
                ax = (6.2831855f * p->rot[0]) / 65536.0f;
                ay = (6.2831855f * p->rot[1]) / 65536.0f;
                az = (6.2831855f * p->rot[2]) / 65536.0f;
                if (o->ownerMotion < thr) {
                    v[0] = p->scale[0];
                    v[1] = p->scale[1];
                    v[2] = p->scale[2];
                } else {
                    v[0] = p->scale[0] * (3.5f - 2.5f * f);
                    v[1] = p->scale[1] * (3.5f - 2.5f * f);
                    v[2] = p->scale[2] * (3.5f - 2.5f * f);
                }
                keys = *keysp;
                Mtx_Scale(m, v[0], v[1], v[2]);
                Mtx_RotateXYZ(m, ax, ay, az);
                Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                if (p->idx == 0) {
                    k0 = 0;
                    Mtx_Translation(tm, 0.125f * o->stateVar0, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    Mtx_Concat3x3(m, gCamMatrix);
                    mode = (u16) (mode | 2);
                    Eff_ColorKeyLerp(keys, o->stateVar0, &color);
                } else {
                    k0 = 1;
                    if (p->idx == 2) {
                        mode = (u16) (mode | 2);
                    }
                    Eff_ColorKeyLerp(keys, o->ownerMotion % 9, &color);
                }
                mdl = &em->matSets[k0];
                color = (alpha << 24) | (((color >> 16) & 0xFF) << 16) | (((color >> 8) & 0xFF) << 8) | (color & 0xFF);
                EffEm08_DrawModel(mdl, m, color, mode, emMats);
                mode = 0;
                p++;
                keysp++;
            }
            break;
        case 1:
            p += n;
            Gs_SetState(0x6C, 0);
            mdl = &set->matSets[59];
            for (i = 0; i < cnt; i++) {
                v[0] = 4.0f * p->scale[0];
                v[1] = 4.0f * p->scale[1];
                v[2] = 4.0f * p->scale[2];
                Mtx_Scale(m, v[0], v[1], v[2]);
                Mtx_RotateX(m, (6.2831855f * p->rot[0]) / 65536.0f);
                Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                EffEm08_DrawModel(mdl, m, ((u8) (0.3f * alpha) << 24) | (r << 16) | (g << 8) | bl, mode, mats);
                mode = 0;
                p++;
            }
            Gs_SetState(0x6C, 1);
            break;
        }
        break;
    case 1:
        Mtx_Identity(m);
        Mtx_RotateXYZ(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
        EffEm08_Billboard(m, o->pos2);
        m[0] *= p->scale[0];
        m[1] *= p->scale[0];
        m[2] *= p->scale[0];
        m[4] *= p->scale[1];
        m[5] *= p->scale[1];
        m[6] *= p->scale[1];
        m[8] *= p->scale[2];
        m[9] *= p->scale[2];
        m[10] *= p->scale[2];
        mdl = &em->matSets[model];
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Gs_SetState(0x6C, 0);
        Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
        Gs_SetState(0x19, (s32) tm);
        EffEm08_DrawModel(mdl, m, -1, 2, emMats);
        break;
    case 2:
    case 3:
    case 9:
    case 13:
    case 22:
    case 23:
    case 24:
        switch (ds->part) {
        case 0:
            break;
        case 1:
            p++;
            break;
        }
        if (p->unk02 > 0 && p->unk02 < 11) {
            Vec_Copy(v, p->scale);
            switch (o->type) {
            case 24:
                v[0] *= EffEm08_K12Scales[o->stateVar1];
                v[1] *= EffEm08_K12Scales[o->stateVar1];
                v[2] *= EffEm08_K12Scales[o->stateVar1];
            case 9:
            case 22:
            case 23:
                mdl = &set->matSets[model];
                v[0] *= 0.5f;
                v[1] *= 0.5f;
                v[2] *= 0.5f;
                break;
            default:
                mats = emMats;
                mdl = &em->matSets[model];
                break;
            }
            switch (o->type) {
            case 2:
                keys = EffEm08_Keys2;
                break;
            case 3:
            case 13:
                keys = EffEm08_Keys13;
                break;
            case 23:
                keys = EffEm08_Keys23;
                break;
            default:
                keys = EffEm08_KeysDef;
                break;
            }
            Eff_ColorKeyLerp(keys, p->unk02, &color);
            Mtx_Scale(m, v[0], v[1], v[2]);
            Mtx_RotateZ(m, (6.2831855f * p->rot[2]) / 65536.0f);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            Mtx_Concat3x3(m, gCamMatrix);
            EffEm08_DrawModel(mdl, m, color, 0, mats);
        }
        break;
    case 4:
        Gs_SetState(0x6C, 0);
        for (i = 0; i < 3; i++, p++) {
            if (p->unk1F == 0) {
                continue;
            }
            flat = 0;
            switch ((u8) b->unk02) {
            default:
                continue;
            case 0:
                switch (p->idx) {
                case 0:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    Eff_ColorKeyLerp(EffEm08_ColorKeysA, p->unk02, &color);
                    break;
                default:
                    continue;
                }
                break;
            case 1:
                switch (p->idx) {
                default:
                    continue;
                case 0:
                    flat = 1;
                    k = 7;
                    break;
                case 1:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    break;
                }
                color = (p->unk01 << 24) | 0xFFFFFF;
                break;
            case 2:
            case 3:
                flag = 0;
                switch (p->idx) {
                default:
                    continue;
                case 1:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    if ((u8) b->unk02 == 3) {
                        flag = 1;
                    }
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 5), 0.0f);
                    break;
                }
                Gs_SetState(0x19, (s32) tm);
                if (flag) {
                    Eff_ColorKeyLerp(EffEm08_ColorKeysB, p->unk02, &color);
                } else {
                    color = (p->unk01 << 24) | 0xFFFFFF;
                }
                break;
            }
            if (flat) {
                Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
                Mtx_Concat3x3(m, gCamMatrix);
            } else {
                Mtx_Scale(m, p->scale[0], p->scale[1], 4.0f);
                Mtx_RotateZXY(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f,
                              (6.2831855f * p->rot[2]) / 65536.0f);
            }
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            EffEm08_DrawModel(&em->matSets[k], m, color, 2, emMats);
        }
        break;
    case 5:
        mdl = &em->matSets[model];
        Mtx_Identity(m);
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Mtx_Concat3x3(m, gCamYawMtx);
        EffEm08_DrawModel(mdl, m, -1, 0, emMats);
        break;
    case 6:
        switch (ds->part) {
        case 0:
            cnt2 = 1;
            break;
        case 1:
            cnt2 = 5;
            p++;
            break;
        }
        Gs_SetState(0x6C, 0);
        for (i = 0; i < cnt2; i++) {
            lim = EffEm08_LimitsByIdx[p->idx];
            if (p->unk02 > 0 && p->unk02 <= lim) {
                Eff_ColorKeyLerp(EffEm08_ColorKeysByIdx[p->idx], p->unk02, &color);
                k = EffEm08_KeysByIdx[p->idx];
                switch (ds->part) {
                case 0:
                    Mtx_Identity(m);
                    Mtx_RotateXYZ(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
                    EffEm08_Billboard(m, o->pos);
                    m[0] *= p->scale[0];
                    m[1] *= p->scale[0];
                    m[2] *= p->scale[0];
                    m[4] *= p->scale[1];
                    m[5] *= p->scale[1];
                    m[6] *= p->scale[1];
                    m[8] *= p->scale[2];
                    m[9] *= p->scale[2];
                    m[10] *= p->scale[2];
                    Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                    mode = (u16) (mode | 2);
                    break;
                case 1:
                    v[0] = p->scale[0];
                    v[1] = p->scale[1];
                    v[2] = p->scale[2];
                    Mtx_Scale(m, v[0], v[1], v[2]);
                    switch (p->idx) {
                    case 3:
                        Gs_SetState(0x60, 0x80);
                        a = (6.2831855f * o->rotY) / 65536.0f;
                        Mtx_RotateZ(m, (6.2831855f * p->rot[2]) / 65536.0f);
                        Mtx_RotateY(m, a);
                        Mtx_Translation(tm, 0.0f, 0.0234375f * p->unk02, 0.0f);
                        Gs_SetState(0x19, (s32) tm);
                        break;
                    default:
                        Gs_SetState(0x60, 0);
                        Mtx_RotateZ(m, (6.2831855f * p->rot[2]) / 65536.0f);
                        Mtx_Concat3x3(m, gCamMatrix);
                        mode = (u16) (mode | 2);
                        break;
                    }
                    Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                    break;
                }
                EffEm08_DrawModel(&em->matSets[k], m, color, mode, emMats);
            }
            p++;
        }
        break;
    case 7:
        Gs_SetState(0x6C, 0);
        for (i = 0; i < 2; i++) {
            switch (p->idx) {
            case 0:
                Mtx_Identity(m);
                Mtx_RotateXYZ(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
                EffEm08_Billboard(m, o->pos);
                m[0] *= p->scale[0];
                m[1] *= p->scale[0];
                m[2] *= p->scale[0];
                m[8] *= 2.0f;
                m[9] *= 2.0f;
                m[10] *= 2.0f;
                Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                model = 5;
            default:
                Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                mdl = &em->matSets[model];
                color = 0xFFD8B68A;
                EffEm08_DrawModel(mdl, m, color, 0, emMats);
                p++;
                break;
            case 1:
                break;
            }
        }
        break;
    case 8:
        if (p->unk02 > 0 && p->unk02 < 9) {
            Gs_SetState(0x6C, 0);
            mdl = &em->matSets[EffEm08_ModelIdx[(u8) b->unk03]];
            Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
            if (o->stateVar0 & 1) {
                Mtx_RotateY(m, 3.1415927f);
            }
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            Eff_ColorKeyLerp(EffEm08_ColorKeysC, p->unk02, &color);
            Mtx_Translation(tm, 0.25f * (u8) b->unk02, 0.125f - 0.015625f * p->unk02, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            Mtx_Concat3x3(m, gCamMatrix);
            EffEm08_DrawModel(mdl, m, color, 2, emMats);
        }
        break;
    case 14:
        Gs_SetState(0x6C, 0);
        Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
        Mtx_RotateXYZ(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        mdl = &em->matSets[model];
        color = -1;
        Mtx_Translation(tm, (o->ownerMotion & 0xF) / 16.0f, 1.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        EffEm08_DrawModel(mdl, m, color, 2, emMats);
        break;
    case 16:
    case 20:
        Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
        Mtx_RotateXYZ(m, (6.2831855f * p->rot[0]) / 65536.0f, (6.2831855f * p->rot[1]) / 65536.0f,
                      (6.2831855f * p->rot[2]) / 65536.0f);
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        if (o->type == 20 && p->idx != 0) {
            mdl = &set->matSets[73];
        } else {
            mdl = &set->matSets[model];
        }
        color = -1;
        EffEm08_DrawModel(mdl, m, color, 0, mats);
        break;
    case 15:
    case 17:
        color = -1;
        switch (ds->part) {
        case 0:
            Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
            Mtx_RotateXYZ(m, (6.2831855f * p->rot[0]) / 65536.0f, (6.2831855f * p->rot[1]) / 65536.0f,
                          (6.2831855f * p->rot[2]) / 65536.0f);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            EffEm08_DrawModel(&em->matSets[model], m, color, 0, emMats);
            break;
        case 1:
            Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            mdl = &set->matSets[50];
            dy = o->pos[1] - ds->pos[1];
            for (j = 0; j < mdl->count; j++) {
                mat = &mats[mdl->mat[j]];
                if (!(o->velY < 0.0f)) {
                    mat->alpha = 0.0f;
                } else if (!(dy < 3500.0f)) {
                    mat->alpha = 0.0f;
                } else if (dy <= 2000.0f) {
                    mat->alpha = 1.0f;
                } else {
                    mat->alpha = 1.0f - (dy - 2000.0f) / 1500.0f;
                }
                Gs_SetState((u8) (j + 0x3A), (s32) mat);
            }
            EffEm08_DrawModel(mdl, m, color, 0, mats);
            break;
        }
        break;
    case 18:
        Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
        Mtx_RotateZXY(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f,
                      (6.2831855f * p->rot[2]) / 65536.0f);
        mdl = &em->matSets[model];
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Gs_SetState(0x6C, 0);
        Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
        Gs_SetState(0x19, (s32) tm);
        EffEm08_DrawModel(mdl, m, -1, 2, emMats);
        break;
    case 19:
        Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
        Mtx_RotateXYZ(m, (6.2831855f * p->rot[0]) / 65536.0f, (6.2831855f * p->rot[1]) / 65536.0f,
                      (6.2831855f * p->rot[2]) / 65536.0f);
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        mdl = &em->matSets[model];
        color = 0xFF808080;
        EffEm08_DrawModel(mdl, m, color, 0, emMats);
        break;
    case 21:
        Gs_SetState(0x6C, 0);
        for (i = 0; i < 3; i++, p++) {
            if (p->unk1F == 0) {
                continue;
            }
            flat2 = 0;
            switch ((u8) b->unk02) {
            default:
                continue;
            case 0:
                switch (p->idx) {
                case 0:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    Eff_ColorKeyLerp(EffEm08_ColorKeysD, p->unk02, &color);
                    break;
                default:
                    continue;
                }
                break;
            case 1:
                switch (p->idx) {
                default:
                    continue;
                case 0:
                    flat2 = 1;
                    k = 4;
                    break;
                case 1:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    break;
                }
                color = (p->unk01 << 24) | 0xFFFFFF;
                break;
            case 2:
            case 3:
                flag = 0;
                switch (p->idx) {
                default:
                    continue;
                case 1:
                    k = 0;
                    Mtx_Translation(tm, 0.0f, 1.0f - 0.125f * (o->ownerMotion & 7), 0.0f);
                    if ((u8) b->unk02 == 3) {
                        flag = 1;
                    }
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 5), 0.0f);
                    break;
                }
                Gs_SetState(0x19, (s32) tm);
                if (flag) {
                    Eff_ColorKeyLerp(EffEm08_ColorKeysB, p->unk02, &color);
                } else {
                    color = (p->unk01 << 24) | 0xFFFFFF;
                }
                break;
            }
            if (flat2) {
                if (o->stateVar1 == 1) {
                    Mtx_Scale(m, 0.5f * p->scale[0], 0.5f * p->scale[1], p->scale[2]);
                } else {
                    Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
                }
                Mtx_Concat3x3(m, gCamMatrix);
            } else {
                if (o->stateVar1 == 1) {
                    Mtx_Scale(m, 0.5f * p->scale[0], 0.5f * p->scale[1], 4.0f);
                } else {
                    Mtx_Scale(m, p->scale[0], p->scale[1], 4.0f);
                }
                Mtx_RotateZXY(m, (6.2831855f * o->rotX) / 65536.0f, (6.2831855f * o->rotY) / 65536.0f,
                              (6.2831855f * p->rot[2]) / 65536.0f);
            }
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            EffEm08_DrawModel(&em->matSets[k], m, color, 2, emMats);
        }
        break;
    }
    Draw_SetDefaultState();
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6C, 1);
end:;
}

void EffEm08_Impact(ObjAttach *o, s16 n, f32 *pos)
{
    f32 scale;
    f32 s;
    s32 k;

    switch (o->type) {
    case 0:
    case 0xB:
    case 0xC:
        if (o->type == 0xB || o->type == 0xC) {
            scale = 2.25f;
            k = 18;
        } else if (o->stateVar1 == 0) {
            scale = 1.5f;
            k = 1;
        } else {
            scale = 2.15f;
            k = 4;
        }
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x25, 0, pos, 15, 0);
            s = 0.15f * scale;
            StageObj_K0E_CreateScaled((f32 *) pos, 1, s);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 4, s);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 9, scale);
        }
        switch (n) {
        case 0:
            if (o->type != 0x18 && o->area == gScene.area) {
                StageObj_K11_CreateAtScaled(pos, o->rotY, 8, scale);
                StageObj_K11_CreateAtScaled(pos, o->rotY, 0xB, scale);
                CamShake_SetA(2, pos);
            }
            Eff09_CreateOwned(o->owner, pos, k);
            break;
        case 2:
            Eff09_CreateOwned(o->owner, pos, k);
            break;
        }
        break;
    case 1:
        if (o->area == gScene.area) {
            if (o->ownerMotion & 2) {
                Em_PlaySeChecked(o->owner, 0x25, 0, pos, 1, 0);
            }
            StageObj_K11_CreateAt(pos, 0xD, ((Em *) o->owner)->type, 1.0f);
        }
        break;
    case 2:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x25, 0, pos, 1, 0);
        }
        if (o->area == gScene.area) {
            StageObj_K11_CreateAtScaled(pos, o->rotY, 5, 1.0f);
            StageObj_K11_CreateAtYaw(pos, 0xF, ((Em *) o->owner)->type, o->rotY, 1.4f);
        }
        if (o->stateVar1 == 1) {
            Eff09_CreateOwned(o->owner, pos, 0x18);
        } else {
            Eff09_CreateOwned(o->owner, pos, 0xB);
        }
        break;
    case 3:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x25, 0, pos, 1, 0);
        }
        if (o->area == gScene.area) {
            StageObj_K11_CreateAtScaled(pos, o->rotY, 0x10, 1.0f);
            StageObj_K11_CreateAtYaw(pos, 0xF, ((Em *) o->owner)->type, o->rotY, 1.2f);
        }
        Eff09_CreateOwned(o->owner, pos, 0xC);
        break;
    case 5:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x1D, 0, pos, 1, 0);
            EmFxSpark_SpawnAt(pos, 1, ((Em *) o->owner)->type, 1.0f);
            EmFxSpark_SpawnAt(pos, 2, ((Em *) o->owner)->type, 1.0f);
        }
        break;
    case 7:
        switch (n) {
        case 0:
            if (o->area == gScene.area) {
                StageObj_K11_CreateAt(pos, 0x14, ((Em *) o->owner)->type, 1.0f);
            }
            Eff09_CreateOwned(o->owner, pos, 0x11);
            break;
        case 1:
        case 2:
            if (o->area == gScene.area) {
                StageObj_K11_CreateAt(pos, 0x13, ((Em *) o->owner)->type, 1.0f);
            }
            break;
        }
        break;
    case 9:
    case 0x16:
        if (o->area == gScene.area) {
            StageObj_K12_CreateAt(pos, 9, 1);
        }
        break;
    case 0x17:
        if (o->area == gScene.area) {
            StageObj_K12_CreateAt(pos, 9, 7);
        }
        break;
    case 0x18:
        if (o->area == gScene.area) {
            StageObj_K12_CreateAtScaled(pos, 9, 1, EffEm08_K12Scales[o->stateVar1]);
        }
        break;
    case 0xD:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x25, 0, pos, 1, 0);
        }
        if (o->area == gScene.area) {
            CamShake_SetA(2, pos);
            StageObj_K0E_CreateScaledB(o->pos, 0, 1.5f);
            StageObj_K11_CreateAtYaw(pos, 0xF, ((Em *) o->owner)->type, o->rotY, 1.8000001f);
        }
        Eff09_CreateOwned(o->owner, pos, 0x19);
        break;
    case 0xF:
        if (o->area == gScene.area) {
            CamShake_SetA(2, pos);
            StageObj_K1C_CreateAt(pos, 5, 1.0f);
            Em_PlaySeChecked(o->owner, 0x17, 0, pos, 15, 0);
        }
        EffEm08_CreateAt2(o->owner, 0x10, 0, o, pos, 0xE000, o->rotY);
        EffEm08_CreateAt2(o->owner, 0x10, 0, o, pos, 0xE000, o->rotY + 0x6000);
        EffEm08_CreateAt2(o->owner, 0x10, 0, o, pos, 0xE000, o->rotY + 0xA000);
        break;
    case 0x12:
        if (o->area == gScene.area) {
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0xE, 1.0f);
            if (n == 0) {
                Em_PlaySeChecked(o->owner, 0x3D, 0, pos, 1, 0);
            }
        }
        break;
    case 0x13:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(o->owner, 0x1C, 0, pos, 1, 0);
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x1E, 2.0f);
        }
        break;
    }
}

void EffEm08_AttachToBone(ObjAttach *o, u8 *b)
{
    f32 v[3];
    f32 m[16];
    f32 z;
    f32 x;

    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, *(s16 *) (b + 0x30))));
    v[0] = EffEm08_BoneOfs[o->type * 3];
    v[1] = D_0089DA54[o->type * 3];
    v[2] = D_0089DA58[o->type * 3];
    Vec_RotateSelf(v, m);
    o->pos[0] = m[12] + v[0];
    o->pos[1] = m[13] + v[1];
    o->pos[2] = m[14] + v[2];
    Vec_Copy(o->pos2, o->pos);
    if (o->type == 1 && o->stateVar1 == 1) {
        x = m[8];
        z = m[10];
        o->rotX = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(-m[9], Math_Sqrtf(x * x + z * z))) / 6.2831855f);
        o->rotX += b[1] << 8;
    } else {
        o->rotY = (b[0x3B] << 8) + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(m[8], m[10])) / 6.2831855f);
    }
}

void EffEm08_LerpPos(ObjAttach *o, ObjAttach *ref, f32 *out)
{
    f32 t;
    f32 x0;
    f32 x1;
    f32 z1;
    f32 dz;

    if (Math_Absf(o->pos[1] - o->pos2[1]) < 0.0001f) {
        out[0] = o->pos[0];
        out[1] = o->pos[1];
        out[2] = o->pos[2];
    } else if (!(ref->pos[1] <= o->pos2[1])) {
        out[0] = o->pos2[0];
        out[1] = o->pos2[1];
        out[2] = o->pos2[2];
    } else {
        f32 *a = o->pos2;
        f32 z0;

        t = (ref->pos[1] - a[1]) / (o->pos[1] - a[1]);
        x0 = a[0];
        x1 = o->pos[0];
        z1 = o->pos[2];
        z0 = a[2];
        dz = t * (z1 - z0);
        out[0] = x0 + t * (x1 - x0);
        out[1] = ref->pos[1];
        out[2] = a[2] + dz;
    }
}

void EffEm08_Billboard(f32 *dst, void *pos)
{
    f32 a[4];
    f32 b[4];
    f32 c[4];
    f32 d[4];

    Vec_Sub(a, &gCamPos, (f32 *) pos);
    Vec_Copy(b, dst + 8);
    Vec_Cross(c, a, b);
    Vec_Normalize(c);
    Vec_Cross(d, b, c);
    if (!(Vec_Dot(d, d) <= 0.0001f)) {
        Vec_Cross(c, d, b);
        Vec_Copy(dst, c);
        Vec_Copy(dst + 4, d);
    }
}

void Eff_ColorKeyLerp(ColorKey *k, s32 t, u32 *out)
{
    f32 f;
    s32 f0;
    s32 f1;
    u32 c0;
    u32 c1;
    u8 a;
    u8 b;
    u8 g;
    u8 r;

    for (;; k++) {
        f0 = k->frame;
        if (f0 == -1) {
            return;
        }
        if (t == f0) {
            *out = k->color;
            return;
        }
        f1 = k[1].frame;
        if (f0 < t && t < f1) {
            f = (f32) (t - f0) / (f32) (f1 - f0);
            c0 = k->color;
            b = c0 >> 16;
            g = c0 >> 8;
            r = c0;
            a = c0 >> 24;
            *out = (u8) (a + (u8) (f * ((f32) (u8) (k[1].color >> 24) - (f32) a))) << 24
                 | (u8) (b + (u8) (f * ((f32) (u8) (k[1].color >> 16) - (f32) b))) << 16
                 | (u8) (g + (u8) (f * ((f32) (u8) (k[1].color >> 8) - (f32) g))) << 8
                 | (u8) (r + (u8) (f * ((f32) (u8) k[1].color - (f32) r)));
            return;
        }
    }
}

void EffEm08_PickDirection(ObjAttach *o, Em08Part *p)
{
    s8 temp_a1;

    Em_PlaySeChecked(o->owner, 0x37, 0, o->pos, 5, 1);
    temp_a1 = p->idx;
    switch (temp_a1) {
    case 0:
        p->unk02 = 1;
        break;
    case 1:
        p->unk02 = 1;
        p->rot[2] = Rand_Next(1);
        break;
    case 2:
        p->unk02 = 1;
        break;
    case 3:
        p->unk02 = 1;
        p->rot[2] = Rand_Next(1);
        break;
    case 4:
        p->unk02 = -1;
        p->rot[2] = Rand_Next(1);
        break;
    }
}

void EffEm08_Respawn(ObjAttach *o, u8 *b)
{
    f32 v[3];
    f32 p[3];
    u8 r;
    f32 (*tbl)[2];

    b[0]--;
    r = Rand_Next(1);
    o->stateVar0 = r & 1;
    b[2] = (r >> 1) % 3;
    b[3] = (r >> 3) % 3;
    if (b[1] != 0) {
        EffEm08_CopyRespawnPos((u8 *) o, b);
        if (gScene.area == o->area) {
            Em_PlaySeChecked(o->owner, 0x18, 0, o->pos, 1, 0);
        }
        if (b[0] == 0) {
            return;
        }
    }
    EffEm08_InitByKind(o, (s32) o->owner);
    tbl = EffEm08_RespawnTbls[o->stateVar1 % 6];
    v[0] = tbl[b[1]][0];
    v[1] = 0.0f;
    v[2] = tbl[b[1]][1];
    Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
    Vec_Add(o->pos, (f32 *) (b + 0xC), v);
    o->pos[1] = AreaGrid2_GetFloorYFiltered(o->pos);
    p[0] = o->pos[0];
    p[1] = 1000.0f + o->pos[1];
    p[2] = o->pos[2];
    if (gScene.area == o->area) {
        EmFxSpark_SpawnAt(p, 6, ((Player *) o->owner)->weaponType, 1.0f);
        EmFxSpark_SpawnAt(o->pos, 7, ((Player *) o->owner)->weaponType, 1.0f);
    }
    b[1]++;
}

void EffEm08_CopyRespawnPos(u8 *a, u8 *b)
{
    Vec_Copy((f32 *) (b + 0x18), (f32 *) (a + 0x30));
}

/* unk_6CADF0's .data (0x89C6F0-0x89DE10) */
HitShape EffEm08_Vals088_0[2] = {
#include "game/0089C6F0.inc"
};
HitShape EffEm08_Vals088_1[2] = {
#include "game/0089C740.inc"
};
HitShape EffEm08_Vals088_2[2] = {
#include "game/0089C790.inc"
};
HitShape EffEm08_Vals088_3[3] = {
#include "game/0089C7E0.inc"
};
HitShape EffEm08_Vals088_4[2] = {
#include "game/0089C860.inc"
};
HitShape EffEm08_Vals088_5[2] = {
#include "game/0089C8B0.inc"
};
HitShape EffEm08_Vals088_6[2] = {
#include "game/0089C900.inc"
};
HitShape EffEm08_Vals088_7[2] = {
#include "game/0089C950.inc"
};
HitShape EffEm08_Vals088_8[2] = {
#include "game/0089C9A0.inc"
};
HitShape EffEm08_Vals088_9[2] = {
#include "game/0089C9F0.inc"
};
HitShape EffEm08_Vals088_10[2] = {
#include "game/0089CA40.inc"
};
HitShape EffEm08_Vals088_11[2] = {
#include "game/0089CA90.inc"
};
HitShape *EffEm08_Vals088[12] = {
    EffEm08_Vals088_0, EffEm08_Vals088_1, EffEm08_Vals088_2, EffEm08_Vals088_3,
    EffEm08_Vals088_4, EffEm08_Vals088_5, EffEm08_Vals088_6, EffEm08_Vals088_7,
    EffEm08_Vals088_8, EffEm08_Vals088_9, EffEm08_Vals088_10, EffEm08_Vals088_11,
};
u32 D_0089CB10[300] = {
#include "game/0089CB10.inc"
};
void *EffEm08_MotionDesc = (void *)D_0089CB10;
u32 EffEm08_Curves_0[20] = {
#include "game/0089CFD0.inc"
};
u32 EffEm08_Curves_1[20] = {
#include "game/0089D020.inc"
};
u32 EffEm08_Curves_2[14] = {
#include "game/0089D070.inc"
};
s16 EffEm08_SpinX[4] = {
#include "game/0089D0A8.inc"
};
s16 EffEm08_SpinY[8] = {
#include "game/0089D0B0.inc"
};
ColorKey EffEm08_KeysP12_0[4] = {
#include "game/0089D0C0.inc"
};
ColorKey EffEm08_KeysP12_1[4] = {
#include "game/0089D0E0.inc"
};
ColorKey EffEm08_KeysP12_2[4] = {
#include "game/0089D100.inc"
};
ColorKey *EffEm08_KeysP12[4] = {
    EffEm08_KeysP12_0, EffEm08_KeysP12_1, EffEm08_KeysP12_2, NULL,
};
u32 EffEm08_Curves_3[16] = {
#include "game/0089D130.inc"
};
u32 EffEm08_Curves_4[20] = {
#include "game/0089D170.inc"
};
ColorKey EffEm08_Keys2[4] = {
#include "game/0089D1C0.inc"
};
ColorKey EffEm08_Keys13[4] = {
#include "game/0089D1E0.inc"
};
u32 EffEm08_Curves_5[16] = {
#include "game/0089D200.inc"
};
ColorKey EffEm08_ColorKeysA[4] = {
#include "game/0089D240.inc"
};
u32 EffEm08_Curves_6[20] = {
#include "game/0089D260.inc"
};
u32 EffEm08_Curves_7[20] = {
#include "game/0089D2B0.inc"
};
u32 EffEm08_Curves_8[20] = {
#include "game/0089D300.inc"
};
ColorKey EffEm08_ColorKeysB[4] = {
#include "game/0089D350.inc"
};
u32 EffEm08_Curves_9[20] = {
#include "game/0089D370.inc"
};
u32 EffEm08_Curves_10[24] = {
#include "game/0089D3C0.inc"
};
u32 EffEm08_Curves_11[20] = {
#include "game/0089D420.inc"
};
u32 EffEm08_Curves_12[12] = {
#include "game/0089D470.inc"
};
u32 EffEm08_Curves_13[12] = {
#include "game/0089D4A0.inc"
};
u32 EffEm08_Curves_14[12] = {
#include "game/0089D4D0.inc"
};
ColorKey EffEm08_ColorKeysByIdx_0[4] = {
#include "game/0089D500.inc"
};
ColorKey EffEm08_ColorKeysByIdx_1[4] = {
#include "game/0089D520.inc"
};
ColorKey EffEm08_ColorKeysByIdx_2[4] = {
#include "game/0089D540.inc"
};
ColorKey EffEm08_ColorKeysByIdx_3[4] = {
#include "game/0089D560.inc"
};
ColorKey EffEm08_ColorKeysByIdx_4[4] = {
#include "game/0089D580.inc"
};
ColorKey *EffEm08_ColorKeysByIdx[6] = {
    EffEm08_ColorKeysByIdx_0, EffEm08_ColorKeysByIdx_1, EffEm08_ColorKeysByIdx_2, EffEm08_ColorKeysByIdx_3,
    EffEm08_ColorKeysByIdx_4, NULL,
};
s16 EffEm08_LimitsByIdx[6] = {
#include "game/0089D5B8.inc"
};
s16 EffEm08_KeysByIdx[6] = {
#include "game/0089D5C8.inc"
};
u32 EffEm08_Curves_16[20] = {
#include "game/0089D5E0.inc"
};
ColorKey EffEm08_ColorKeysC[6] = {
#include "game/0089D630.inc"
};
ColorKey EffEm08_KeysDef[4] = {
#include "game/0089D660.inc"
};
ColorKey EffEm08_Keys23[4] = {
#include "game/0089D680.inc"
};
ColorKey EffEm08_KeysP12B_0[4] = {
#include "game/0089D6A0.inc"
};
ColorKey EffEm08_KeysP12B_1[4] = {
#include "game/0089D6C0.inc"
};
ColorKey EffEm08_KeysP12B_2[4] = {
#include "game/0089D6E0.inc"
};
ColorKey *EffEm08_KeysP12B[4] = {
    EffEm08_KeysP12B_0, EffEm08_KeysP12B_1, EffEm08_KeysP12B_2, NULL,
};
u32 EffEm08_Curves_17[12] = {
#include "game/0089D710.inc"
};
ColorKey EffEm08_ColorKeysD[4] = {
#include "game/0089D740.inc"
};
u32 EffEm08_Curves[20] = {
    (u32)EffEm08_Curves_0, (u32)EffEm08_Curves_1, (u32)EffEm08_Curves_2, (u32)EffEm08_Curves_3,
    (u32)EffEm08_Curves_4, (u32)EffEm08_Curves_5, (u32)EffEm08_Curves_6, (u32)EffEm08_Curves_7,
    (u32)EffEm08_Curves_8, (u32)EffEm08_Curves_9, (u32)EffEm08_Curves_10, (u32)EffEm08_Curves_11,
    (u32)EffEm08_Curves_12, (u32)EffEm08_Curves_13, (u32)EffEm08_Curves_14, (u32)EffEm08_Curves_14,
    (u32)EffEm08_Curves_16, (u32)EffEm08_Curves_17, 0, 0,
};
s16 EffEm08_PartCounts[32] = {
#include "game/0089D7B0.inc"
};
s16 EffEm08_Limits[32] = {
#include "game/0089D7F0.inc"
};
s16 EffEm08_Lives[32] = {
#include "game/0089D830.inc"
};
s16 EffEm08_KeyIdx[32] = {
#include "game/0089D870.inc"
};
s16 EffEm08_Steps[32] = {
#include "game/0089D8B0.inc"
};
s16 EffEm08_Models[32] = {
#include "game/0089D8F0.inc"
};
s16 EffEm08_MotionBase[32] = {
#include "game/0089D930.inc"
};
f32 EffEm08_Val050ByType[28] = {
#include "game/0089D970.inc"
};
f32 EffEm08_Val058ByType[28] = {
#include "game/0089D9E0.inc"
};
f32 EffEm08_BoneOfs[76] = {
#include "game/0089DA50.inc"
};
s16 EffEm08_BoneIdx[8] = {
#include "game/0089DB80.inc"
};
u32 D_0089DB90[8] = {
#include "game/0089DB90.inc"
};
u32 D_0089DBB0[12] = {
#include "game/0089DBB0.inc"
};
u32 D_0089DBE0[16] = {
#include "game/0089DBE0.inc"
};
u32 D_0089DC20[12] = {
#include "game/0089DC20.inc"
};
u32 D_0089DC50[16] = {
#include "game/0089DC50.inc"
};
u32 D_0089DC90[12] = {
#include "game/0089DC90.inc"
};
f32 (*EffEm08_RespawnTbls[6])[2] = {
    (void *)D_0089DB90, (void *)D_0089DBB0, (void *)D_0089DBE0, (void *)D_0089DC20,
    (void *)D_0089DC50, (void *)D_0089DC90,
};
s16 EffEm08_ModelIdx[4] = {
#include "game/0089DCD8.inc"
};
f32 EffEm08_StartPos[72] = {
#include "game/0089DCE0.inc"
};
f32 EffEm08_K12Scales[4] = {
#include "game/0089DE00.inc"
};
