/*
 * Game overlay code at 0x6C7AE0-0x6CADF0: the BowgunShot_* and EmImpactAttack_* effect
 * objects.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "capsule.h"
#include "stageobj.h"
#include "sprite.h"

/* prototypes */
void BowgunShot_CalcPower(ObjAttach *o);
void BowgunShot_ApplyKind(ObjAttach *o);
s32 BowgunShot_UpdatePowerStage(ObjAttach *o);
void BowgunShot_PartAlloc(ObjAttach *o);
f32 BowgunShot_BitsToFrac(u16 arg0, s32 arg1);
f32 BowgunShot_BitsToSignedFrac(u16 value, u16 bits);
void Explosion_CreateInArea(Player *owner, f32 *pos, s32 kind, u16 area);
u16 BowgunShot_MulAddShift(u16 arg0, u16 arg1, s32 arg2, s32 arg3);
extern u16 BowgunShot_LifeLimit[];
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
void CamShake_SetA(s32 kind, f32 *pos);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
void StageObj_K11_CreateAt(f32 *pos, s16 k, u8 c, f32 scale);
void StageObj_K11_CreateAtYaw(f32 *pos, s16 k, u8 c, s32 v, f32 scale);
void StageObj_K12_CreateAtScaled(f32 *pos, s32 k, u8 c, f32 scale);
void StageObj_K0E_CreateScaledB(f32 *pos, s16 k, f32 scale);
void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f);
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
extern f32 EmRangedAttack_K12Scales[];
s32 Player_IsLocal(void *obj);
s32 Em_HitQuadsVertical(f32 *p, Em **hitEm, s16 *hitBone, f32 *hit);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void PlacedItem_Update(ObjAttach *o);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void Mtx_Inverse(f32 *out, f32 *in);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void BowgunShot_Scatter(ObjAttach *o, u8 *p, f32 f);
void StageObj_K0C_CreateFromAttach(ObjAttach *oa, s32 k, u8 c);
extern s32 BowgunShot_ScatterParams[];
typedef struct { s32 frame; u32 color; } ColorKey;
void BowgunShot_PlaySe(ObjAttach *o, s16 kind);
void StageObj_K12_CreateBetween(u8 *src, s32 k, u8 c);
s32 func_364AE0(ObjAttach *o, u16 x);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
extern u8 BowgunShot_RowsPtr;
extern HitShape *BowgunShot_HitShapes[];
void Mtx_RotateZ(f32 *m, f32 a);
extern s32 BowgunShot_BlendStates[];
void BowgunShot_DrawStageModel(ObjAttach *o);
void EmRangedAttack_CopyRespawnPos(u8 *a, u8 *b);
void EmFxSpark_SpawnAt(void *src, u8 k, u8 c, f32 f);
void Vec_Add(f32 *, f32 *, f32 *);
extern f32 (*EmRangedAttack_RespawnTbls[])[2];
void Vec_RotateSelf(f32 *v, f32 *m);
extern f32 EmRangedAttack_BoneOfs[];
extern f32 D_0089DA54[];
extern f32 D_0089DA58[];
s32 LightBowgun_HasSilencer(Player *player);
s16 Bowgun_RecoilLevel(Player *player, u8 kind);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
extern s16 BowgunShot_Kind15Tbl[];
extern s16 BowgunShot_Kind16Tbl[];
extern f32 BowgunShot_KindScale[];
extern s16 BowgunShot_KindUnk06B[];
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_RotateY(f32 *v, f32 angle);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
extern s16 *BowgunShot_PowerTbls[];
void EmImpactAttack_Update(ObjAttach *o);
u8 Equip_GetClass(EquipItem *e);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void BowgunShot_Init(ObjAttach *o);
void BowgunShot_Draw(DrawSlot *slot);
void BowgunShot_PartDraw(DrawSlot *d);
void BowgunShot_PartRun(ObjAttach *o);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Gfx_SetBlend(s32 a, s32 b);
extern f32 gCamMatrix[];
extern f32 BowgunShot_PartScaleA[][3];
extern f32 BowgunShot_PartScaleB[][3];
extern f32 BowgunShot_PartColors[][3];
void StageObj_K12_CreateAt(f32 *pos, s32 k, u8 c);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_TurnZ(f32 *m, f32 a);
extern HitShape *EmImpactAttack_HitShapes[];
extern void *EmImpactAttack_RowsPtr;
f32 Math_Absf(f32 x);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
s32 Player_HasSkill(Player *p, u8 skill);
u16 Rand_Next(s32 stream);
void BowgunShot_Run(ObjAttach *);
u8 BowgunShot_BounceStep(ObjAttach *);
void BowgunShot_Fall(ObjAttach *);
extern BowgunShotParam gBowgunShotParams[];
void BowgunShot_End(ObjAttach *o);
void BowgunShot_Free(ObjAttach *o);
void BowgunShot_PartFree(ObjAttach *o);
void EmImpactAttack_Run(ObjAttach *o);
void EmImpactAttack_End(ObjAttach *o);
void EmImpactAttack_Free(ObjAttach *o);
void EmRangedAttack_Setup(ObjAttach *o);
void EmRangedAttack_Run(ObjAttach *o);
void EmRangedAttack_Hit(ObjAttach *);
void EmRangedAttack_End(ObjAttach *o);
void EmRangedAttack_Free(ObjAttach *o);
void Explosion_Setup(ObjAttach *o);
void Explosion_Run(ObjAttach *);
void Explosion_Hit(ObjAttach *o);
void Explosion_End(ObjAttach *o);
void Explosion_Free(ObjAttach *o);
void PlacedItem_Setup(ObjAttach *o);
void PlacedItem_Run(ObjAttach *o);
void PlacedItem_Hit(ObjAttach *o);
void PlacedItem_End(ObjAttach *o);
void PlacedItem_Free(ObjAttach *o);
s32 Gs_SetState(u8 reg, u32 v);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *out, f32 *in);
void Draw_SetBlendFlags(u16 flags);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32);
struct MoveObj;
void MoveObj_AddVelocity(struct MoveObj *);
void MoveObj_Integrate(struct MoveObj *);
void EmRangedAttack_Update(ObjAttach *o);
void Vec_Normalize(f32 *);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *, f32 *, f32 *);
void Vec_Sub(f32 *, f32 *, f32 *);
extern f32 gCamPos;
ObjAttach *ObjAttach_Alloc(s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Em_BonePos(void *obj, s16 bone, f32 *out);
u8 Player_InCurrentArea(void *obj);
struct MoveObj;
void MoveObj_StartMotionEntry(struct MoveObj *o, s32 a, void *b);
s32 Player_TestSetFlags(void *, s32 mask);
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *Explosion_HitShapes[];
extern void *Explosion_RowsPtr;
extern u8 Explosion_TakesOwnerSide[];
extern s16 EmRangedAttack_MotionBase[];
extern void *EmRangedAttack_RowsPtr;
void EmRangedAttack_InitByKind(ObjAttach *, s32);
void Explosion_Init(ObjAttach *);
void Explosion_BoostPower(ObjAttach *);
void Explosion_Spawn(ObjAttach *);
void Explosion_Update(ObjAttach *);
void EmImpactAttack_Setup(ObjAttach *o);
void BowgunShot_Setup(ObjAttach *o);
void BowgunShot_DrawModel(DrawSlot *d);
void BowgunShot_State5(ObjAttach *o);
void BowgunShot_Hit(ObjAttach *o);
void EmRangedAttack_Impact(ObjAttach *o, s16 n, f32 *pos);
void BowgunShot_Fly(ObjAttach *o);
/* end of prototypes */

extern f32 D_0045EB10;
extern f32 D_0045EB30;
extern f32 D_006AEE10;
void BowgunShot_Fly(ObjAttach *o)
{
    f32 mtx[16];
    f32 acc[3];
    f32 bp2[3];
    f32 rv[3];
    f32 bpos[3];
    f32 dir[3];
    f32 bp[3];
    f32 v[3];
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    BowgunShotParam *e = &gBowgunShotParams[o->type];
    Em *m;
    f32 s;
    f32 f;
    f32 g;
    s32 r;
    s32 ang;
    HitShape *shape;

    if (o->stateVar0 > 0) {
        switch (e->hitKind) {
        case 0:
        case 2:
        case 7:
            break;
        case 1:
            if (o->ownerMotion >= e->count) {
                o->state = ATTACH_END;
                goto tail;
            }
            if (o->targetEm == 0) {
                o->state = ATTACH_END;
                goto tail;
            }
            m = o->targetEm;
            if (m->taskPhase == 3 || m->active == 0) {
                o->state = ATTACH_END;
                goto tail;
            }
            Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(m, o->aux)));
            Mtx_GetTrans(bp2, mtx);
            Vec_Rotate(rv, &o->velX, mtx);
            o->pos[0] = rv[0] + bp2[0];
            o->pos[1] = rv[1] + bp2[1];
            o->pos[2] = rv[2] + bp2[2];
            goto tail;
        case 3:
        case 4:
        case 5:
            if (o->ownerMotion >= e->count) {
                if (o->area == gScene.area) {
                    StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                }
                switch (e->hitKind) {
                case 3:
                    Explosion_CreateInArea(o->owner, o->pos, 8, o->area);
                    break;
                case 4:
                    Explosion_CreateInArea(o->owner, o->pos, 0xE, o->area);
                    break;
                case 5:
                    Explosion_CreateInArea(o->owner, o->pos, 0xF, o->area);
                    break;
                }
                o->state = ATTACH_END;
                goto tail;
            }
            if (o->targetEm == 0) {
                o->state = ATTACH_END;
                goto tail;
            }
            m = o->targetEm;
            if (m->taskPhase == 3 || m->active == 0) {
                o->state = ATTACH_END;
                goto tail;
            }
            Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(m, o->aux)));
            Mtx_GetTrans(bp2, mtx);
            Vec_Rotate(rv, &o->velX, mtx);
            o->pos[0] = rv[0] + bp2[0];
            o->pos[1] = rv[1] + bp2[1];
            o->pos[2] = rv[2] + bp2[2];
            goto tail;
        case 8:
            if (buf->unk03 != 0 && --buf->unk03 == 0) {
                MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, &BowgunShot_RowsPtr);
                BowgunShot_CalcPower(o);
                o->hitList = BowgunShot_HitShapes[0];
                o->hurtList = 0;
                o->velX *= 0.7f;
                o->velY *= 0.7f;
                o->velZ *= 0.7f;
                BowgunShot_ApplyKind(o);
            }
            break;
        case 6:
            switch (buf->flyStep) {
            case 0:
                if (o->targetEm == 0) {
                    o->state = ATTACH_END;
                    o->hitPhase = 0;
                    goto tail;
                }
                shape = o->targetPart;
                if (shape == NULL || *(u8 *) o->targetEm == 0) {
                    o->state = ATTACH_END;
                    o->hitPhase = 0;
                    goto tail;
                }
                switch (shape->kind) {
                case 0:
                    bp[0] = shape->a[0];
                    bp[1] = o->targetPart->a[1];
                    bp[2] = o->targetPart->a[2];
                    break;
                case 1:
                    bp[0] = 0.5f * (shape->a[0] + shape->b[0]);
                    bp[1] = 0.5f * (o->targetPart->a[1] + o->targetPart->b[1]);
                    bp[2] = 0.5f * (o->targetPart->a[2] + o->targetPart->b[2]);
                    break;
                }
                Mtx_Copy((f32 *) mtx, (f32 *) (Em_GetBone(o->targetEm, o->targetPart->bone)));
                Vec_RotateSelf(bp, mtx);
                Mtx_GetTrans(bpos, mtx);
                bp[0] += bpos[0];
                bp[1] += bpos[1];
                bp[2] += bpos[2];
                dir[0] = o->pos[0] - bp[0];
                dir[1] = o->pos[1] - bp[1];
                dir[2] = o->pos[2] - bp[2];
                Vec_Normalize(dir);
                Vec_Copy(v, &o->velX);
                v[0] *= 0.6f;
                v[1] *= 0.6f;
                v[2] *= 0.6f;
                f = -2.0f * Vec_Dot(v, dir);
                dir[0] *= f;
                dir[1] *= f;
                dir[2] *= f;
                o->velX += dir[0];
                o->velY += dir[1];
                o->velZ += dir[2];
                s = Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ);
                r = (u8) BowgunShot_MulAddShift(o->randBits, o->aux, 0x14D, 2) - 0x80;
                ang = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(o->velY, s)) / 6.2831855f);
                o->rotX = ang + r;
                o->rotY = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(o->velX, o->velZ)) / 6.2831855f)
                          + ((u8) BowgunShot_MulAddShift(o->randBits, o->aux, 0x845, 5) - 0x80);
                buf->flyStep++;
            case 1:
                if (buf->unk03 != 0 && --buf->unk03 == 0) {
                    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, &BowgunShot_RowsPtr);
                    BowgunShot_CalcPower(o);
                    o->hitList = BowgunShot_HitShapes[0];
                    o->hurtList = 0;
                }
                break;
            }
            break;
        }
    }
    Vec_Copy(o->pos2, o->pos);
    switch (e->kind) {
    case 0:
    case 1:
    default:
        s = Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ);
        acc[1] = e->wobbleY * s * BowgunShot_BitsToFrac(BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x147F, 2), 0xE);
        if (o->ownerMotion < 20 && BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x3045, 1) < 0x2000) {
            g = *(f32 *) (buf->levelRow + 0x10);
            if (buf->level & 0x10) {
                g += D_006AEE10;
            }
            if (buf->level & 0x20) {
                if (buf->unk27 == 0) {
                    g += D_0045EB30;
                } else {
                    g += D_0045EB10;
                }
            }
            g *= e->wobbleX;
            if (buf->skills & 0x10) {
                g *= 2.0f;
            }
            if (buf->skills & 0x20) {
                g *= 0.5f;
            }
            acc[0] = g * BowgunShot_BitsToSignedFrac(BowgunShot_MulAddShift(o->randBits, o->ownerMotion, 0x3121, 4), 6);
        } else {
            acc[0] = 0.0f;
        }
        acc[2] = 0.0f;
        Vec_RotateX(acc, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(acc, (6.2831855f * o->rotY) / 65536.0f);
        Vec_Add(&o->velX, &o->velX, acc);
        break;
    case 4:
        if (o->ownerMotion > 22 || buf->stage == 0xFF) {
            o->hitFrames = 0;
            o->state = ATTACH_END;
            goto tail;
        }
        if (o->ownerMotion > 2 && (o->ownerMotion & 3) == 0) {
            buf->unk03++;
            if (buf->unk03 <= e->count) {
                MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, o->type + 1, &BowgunShot_RowsPtr);
                BowgunShot_CalcPower(o);
                o->hitList = BowgunShot_HitShapes[buf->unk03];
                o->hurtList = 0;
            }
        }
        break;
    case 2:
        break;
    }
    if (o->ownerMotion < e->boostFrames) {
        MoveObj_AddVelocity((struct MoveObj *)o);
    } else {
        MoveObj_Integrate((struct MoveObj *)o);
    }
    o->hitFrames = 99;
    if (e->kind != 2 || o->ownerMotion >= 10) {
        g = AreaGrid2_GetFloorYFiltered(o->pos);
        if (e->kind == 4) {
            g -= 100.0f;
        }
        if (o->pos[1] <= g) {
            o->hitFrames = 0;
            o->state = ATTACH_END;
            switch (e->hitKind) {
            case 7:
                if (buf->stage != 0xFF) {
                    BowgunShot_Scatter(o, (u8 *) &BowgunShot_ScatterParams[e->count], buf->scale);
                }
                break;
            case 2:
                Explosion_CreateInArea(o->owner, o->pos, 7, o->area);
                if (o->area == gScene.area) {
                    StageObj_K0E_CreateScaled(o->pos, 0, 1.0f);
                }
                break;
            case 10:
                StageObj_K0C_CreateFromAttach(o, 1, 1);
                break;
            }
        }
    }
    if (BowgunShot_UpdatePowerStage(o)) {
        BowgunShot_ApplyKind(o);
    }
tail:
    switch (buf->phase) {
    case 0:
        if (o->ownerMotion > BowgunShot_LifeLimit[e->life] || o->stateVar0 > 0) {
            buf->phase++;
            if (o->area == gScene.area) {
                StageObj_K12_CreateBetween((u8 *) o, 4, 0);
            }
        }
        break;
    case 1:
        buf->phase++;
        break;
    }
    if (e->unk06 != 0) {
        switch (buf->unk17) {
        case 0:
            buf->unk17++;
            BowgunShot_PartAlloc(o);
            break;
        case 1:
            if (e->hitKind == 8 || e->hitKind == 6) {
                if (o->stateVar0 >= e->count) {
                    buf->unk17++;
                } else {
                    BowgunShot_PartRun(o);
                }
            } else if (o->stateVar0 > 0) {
                buf->unk17++;
            } else {
                BowgunShot_PartRun(o);
            }
            break;
        }
    }
}

void BowgunShot_Hit(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    BowgunShotParam *e = &gBowgunShotParams[o->type];
    f32 m[16];
    f32 inv[16];
    f32 p[3];
    f32 d[3];
    f32 c[3];

    if (o->stateVar1 == 0xFF) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
        goto draw;
    }
    buf->flyStep = 0;
    switch (buf->phase) {
    case 0:
        buf->phase++;
        if (o->area == gScene.area) {
            StageObj_K12_CreateBetween((u8 *) o, 4, 0);
        }
        break;
    case 1:
        buf->phase++;
        break;
    }
    switch (e->hitKind) {
    case 0:
    case 2:
    default:
        o->state = ATTACH_END;
        goto draw;
    case 1:
    case 3:
    case 4:
    case 5:
        o->ownerMotion = 0;
        o->stateVar0++;
        o->hitPhase = 0;
        buf->bounced++;
        if (o->targetEm == 0) {
            o->state = ATTACH_END;
            return;
        }
        if (o->targetPart == 0 || *(u8 *) o->targetEm == 0) {
            o->state = ATTACH_END;
            return;
        }
        o->aux = o->targetPart->bone;
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->targetEm, o->aux)));
        Mtx_GetTrans(p, m);
        Vec_Add(c, o->pos, o->pos2);
        Vec_Scale(c, c, 0.5f);
        d[0] = c[0] - p[0];
        d[1] = c[1] - p[1];
        d[2] = c[2] - p[2];
        Mtx_Inverse(inv, m);
        Vec_Rotate(&o->velX, d, inv);
        o->state = ATTACH_RUN;
        goto draw;
    case 8:
        if (++o->stateVar0 < e->count) {
            buf->unk03 = 2;
        } else {
            o->hitPhase = 0;
        }
        o->state = ATTACH_RUN;
        break;
    case 7:
        BowgunShot_Scatter(o, (u8 *) &BowgunShot_ScatterParams[e->count], buf->scale);
        o->hitPhase = 0;
        o->state = ATTACH_END;
        goto draw;
    case 6:
        if (++o->stateVar0 < e->count) {
            buf->unk03 = 4;
        } else {
            o->hitPhase = 0;
        }
        o->state = ATTACH_RUN;
        break;
    case 9:
        o->state = ATTACH_RUN;
        break;
    case 10:
        StageObj_K0C_CreateFromAttach(o, 1, 1);
        o->hitPhase = 0;
        o->state = ATTACH_END;
        goto draw;
    }
    if (++o->ownerMotion > 120) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
        return;
    }
    BowgunShot_Fly(o);
draw:
    if (o->drawSlotBuf != NULL) {
        if (e->model == 0x62) {
            Vec_Copy(o->drawSlotBuf->pos, buf->drawPos);
        } else {
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
        }
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void BowgunShot_State5(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    Em *owner = o->owner;
    Em *hit = o->targetEm;
    f32 p[3];
    f32 v[3];
    u16 ax;
    u16 ay;

    switch (b->phase) {
    case 0:
        b->phase++;
        if (o->area == gScene.area) {
            StageObj_K12_CreateBetween((u8 *) o, 4, 0);
        }
        break;
    case 1:
        b->phase++;
        break;
    }
    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->side = 0;
    if (func_364AE0(o, o->randBits) == 0) {
        o->stateVar1 = 0xFE;
        o->hitPhase = 0;
        Vec_Diff(v, o->pos2, o->pos);
        Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
        ax += -(Rand_Next(1) & 0x7FF) - 0x300;
        ay += (Rand_Next(1) & 0x1FFF) - 0x1000;
        o->velX = 0.0f;
        o->velY = Rand_Next(1) & 7;
        o->velZ = 10.0f + (Rand_Next(1) & 7);
        o->homingTurn = 0.0f;
        o->gravity = -1.5f - (Rand_Next(1) & 0x1F) / 32.0f;
        o->homingSpeed = 0.0f;
        Vec_RotateX(&o->velX, (6.2831855f * ax) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * ay) / 65536.0f);
    } else {
        o->stateVar1 = 0xFF;
        o->idx = hit->index;
        o->isEm = hit->isEm;
        o->owner = hit;
        Player_TestSetFlags(o, 0);
        MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) hit, 0x22, &BowgunShot_RowsPtr);
        o->hitList = BowgunShot_HitShapes[o->hitListNo];
        o->hurtList = 0;
        Vec_Copy(b->drawPos, o->pos);
        Vec_Copy(p, &owner->posX);
        p[1] += 100.0f;
        Vec_Diff(v, p, o->pos);
        Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
        o->rotX = ax;
        o->rotY = ay;
        o->yaw = o->rotY;
        o->velX = 0.0f;
        o->velY = 0.0f;
        o->velZ = 100.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        o->homingTurn = 0.0f;
        o->gravity = -1.5f;
        o->homingSpeed = 0.0f;
        Vec_Copy(o->pos2, o->pos);
        BowgunShot_PlaySe(o, 2);
    }
    o->state = ATTACH_RUN;
    BowgunShot_Run(o);
}

void BowgunShot_Fall(ObjAttach *o)
{
    Vec_Copy(o->pos2, o->pos);
    o->hitFrames = 0x63;
    if (o->ownerMotion < 0x14) {
        MoveObj_AddVelocity((struct MoveObj *)o);
    } else {
        MoveObj_Integrate((struct MoveObj *)o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
    }
}

u8 BowgunShot_BounceStep(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;

    o->hitPhase = 0;
    buf->bounced = 1;
    if (++o->ownerMotion > 30) {
        return 1;
    }
    MoveObj_Integrate((struct MoveObj *)o);
    o->rotX -= 0xC00;
    o->rotY = 0x400;
    return 0;
}

void BowgunShot_DrawModel(DrawSlot *d)
{
    f32 m[16];
    f32 m1[16];
    ObjAttach *o = d->owner;
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    BowgunShotParam *e = &gBowgunShotParams[o->type];
    Model *set;
    ModelMatSet *mdl;
    ModelMat *tex;
    f32 ax;
    f32 ay;

    if (o->area != gScene.area) {
        return;
    }
    switch (b->phase) {
    case 0:
        BowgunShot_DrawStageModel(o);
        return;
    case 1:
        BowgunShot_DrawStageModel(o);
        break;
    }
    if (e->model == 0xFF) {
        return;
    }
    set = gCommonModels[0];
    if (set == NULL) {
        return;
    }
    if (set->active == 0) {
        return;
    }
    tex = set->mats;
    if (e->model == 0x62) {
        if (o->ownerMotion > 14) {
            return;
        }
        Mtx_Translation(m1, 0.5f, 0.375f, 0.0f);
        Gs_SetState(0x19, (s32) m1);
        Mtx_Scale(m, 2.25f, 2.25f, 2.25f);
        mdl = &set->matSets[e->model] + o->ownerMotion;
        Mtx_RotateZ(m, (6.2831855f * o->rotZ) / 65536.0f);
    } else {
        Mtx_Identity(m);
        mdl = &set->matSets[e->model];
    }
    if (b->bounced == 0) {
        ax = Math_Atan2(-o->velY, Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ));
        ay = Math_Atan2(o->velX, o->velZ);
        o->rotX = (u16) (s32) (0.5f + (65536.0f * ax) / 6.2831855f);
        o->rotY = (u16) (s32) (0.5f + (65536.0f * ay) / 6.2831855f);
    }
    Mtx_RotateX(m, (6.2831855f * o->rotX) / 65536.0f);
    Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
    Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
    Gs_SetState(0x67, BowgunShot_BlendStates[(s8) (u8)e->blend]);
    Gs_SetState(0x1A, (s32) m);
    Gs_SetState(0x60, 0x80);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(tex, mdl);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
}

void BowgunShot_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        BowgunShot_Setup(o);
        break;
    case ATTACH_RUN:
        BowgunShot_Run(o);
        break;
    case ATTACH_END:
        BowgunShot_End(o);
        break;
    case ATTACH_HIT:
        BowgunShot_Run(o);
        break;
    case 4:
        BowgunShot_End(o);
        break;
    case ATTACH_DEFLECTED:
        BowgunShot_State5(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        BowgunShot_State5(o);
        break;
    case 7:
        BowgunShot_End(o);
        break;
    case ATTACH_FREE:
        BowgunShot_Free(o);
        
    default:
        break;
    }
}

void BowgunShot_Setup(ObjAttach *o)
{
    BowgunShot_Init(o);
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = BowgunShot_Draw;
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    } else {
        ObjAttach_Free(o);
    }
}

void BowgunShot_Run(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    BowgunShotParam *e = &gBowgunShotParams[o->type];

    if (o->stateVar1 == 0xFE) {
        if (BowgunShot_BounceStep(o) != 0) {
            o->state = ATTACH_END;
            return;
        }
    } else {
        if (o->state == ATTACH_HIT) {
            BowgunShot_Hit(o);
            return;
        }
        if (++o->ownerMotion > 120) {
            o->hitFrames = 0;
            o->state = ATTACH_END;
            return;
        }
        if (o->stateVar1 != 0xFF) {
            BowgunShot_Fly(o);
        } else {
            BowgunShot_Fall(o);
        }
    }
    if (o->drawSlotBuf != NULL) {
        if (e->model == 0x62) {
            Vec_Copy(o->drawSlotBuf->pos, buf->drawPos);
        } else {
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
        }
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void BowgunShot_End(ObjAttach *o)
{
    BowgunShotBuf *w;

    w = (BowgunShotBuf *) o->buf;
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
    if ((s32) w->unk17 > 0) {
        BowgunShot_PartFree(o);
    }
}

void BowgunShot_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void BowgunShot_Draw(DrawSlot *slot)
{
    if (((ObjAttach *)slot->owner)->area == gScene.area) {
        BowgunShot_DrawModel(slot);
    }
}

void BowgunShot_InitWeapon(BowgunShotBuf *b, Player *pl)
{
    b->attackFactor = pl->attackFactor;
    b->level = pl->equip[6].level;
    b->levelRow = gGunLevelRows[gGunData[pl->equip[6].id].levelRow].bytes + (b->level & 0xF) * 24;
    if (Equip_GetClass(&pl->equip[6]) == 1) {
        b->unk27 = 0;
    } else {
        b->unk27 = 1;
    }
}

void BowgunShot_InitSkills(BowgunShotBuf *b, Player *pl)
{
    b->skills = 0;
    if (Player_HasSkill(pl, 0x36) == 1) {
        b->skills |= 1;
    }
    if (Player_HasSkill(pl, 0x37) == 1) {
        b->skills |= 2;
    }
    if (Player_HasSkill(pl, 0x38) == 1) {
        b->skills |= 4;
    }
    if (Player_HasSkill(pl, 0x47) == 1) {
        b->skills |= 8;
    }
    if (Player_HasSkill(pl, 0xBA) == 1) {
        b->skills |= 0x10;
    }
    if (Player_HasSkill(pl, 0xB9) == 1) {
        b->skills |= 0x20;
    }
}

void BowgunShot_CalcPower(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    f32 f;
    u16 v;

    if (o->attackKind == 8) {
        return;
    }
    f = b->attackFactor * b->scale;
    switch (o->type) {
    case 0:
    case 1:
    case 2:
        if (b->skills & 1) {
            f *= 1.1f;
        }
        break;
    case 3:
    case 4:
    case 5:
        if (b->skills & 2) {
            f *= 1.1f;
        }
        break;
    case 6:
    case 7:
    case 8:
        if (b->skills & 4) {
            f *= 1.3f;
        }
        break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        break;
    case 15:
    case 16:
    case 17:
    case 18:
        v = o->elementPower * f;
        if (v == 0) {
            v = 1;
        } else if (v > 0xFF) {
            v = 0xFF;
        }
        o->elementPower = v;
        break;
    case 19:
    case 20:
    case 21:
        break;
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        if (b->skills & 8) {
            v = 1.125f * o->elementPower;
            if (v > 0xFF) {
                v = 0xFF;
            }
            o->elementPower = v;
        }
        break;
    }
    if (o->power != 0) {
        if (b->crit == 1) {
            f *= 1.25f;
            o->hitFx |= HITFX_CRITICAL;
        } else if (b->crit == 2) {
            f *= 0.5f;
            o->hitFx |= HITFX_WEAK;
        }
        v = o->power * f;
        if (v == 0) {
            v = 1;
        } else if (v > 0xFF) {
            v = 0xFF;
        }
        o->power = v;
    }
}

extern f32 D_0045EB14;
extern f32 D_0045EB34;
extern f32 D_006AEE14;
s32 BowgunShot_UpdatePowerStage(ObjAttach *o)
{
    BowgunShotParam *e = &gBowgunShotParams[o->type];
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    s32 ret = 0;
    s16 *tbl;
    f32 f;
    s16 t;

    if (e->powerTbl == -1) {
        return 0;
    }
    tbl = BowgunShot_PowerTbls[e->powerTbl];
    f = *(f32 *) (buf->levelRow + 0x14);
    if (buf->level & 0x10) {
        f += D_006AEE14;
    }
    if (buf->level & 0x20) {
        if (buf->unk27 == 0) {
            f += D_0045EB34;
        } else {
            f += D_0045EB14;
        }
    }
    while (buf->stage != 0xFF) {
        t = f * tbl[buf->stage];
        if (!(t < o->ownerMotion)) {
            break;
        }
        ret = 1;
        if (t < 0) {
            buf->stage = 0xFF;
            o->hitPhase = 0;
            break;
        }
        buf->stage++;
    }
    return ret;
}

void BowgunShot_ApplyKind(ObjAttach *o)
{
    BowgunShotBuf *b = (BowgunShotBuf *) o->buf;
    s16 *tbl;
    f32 v;

    if (o->power == 0 || b->stage > 4 || b->stage == 0xFF) {
        return;
    }
    switch (o->hitFx & 0x3F) {
    case 9:
    case 11:
    case 13:
    case 15:
        tbl = BowgunShot_Kind15Tbl;
        break;
    case 10:
    case 12:
    case 14:
    case 16:
        tbl = BowgunShot_Kind16Tbl;
        break;
    default:
        return;
    }
    o->hitFx = tbl[b->stage];
    if (b->crit == 1) {
        o->hitFx |= HITFX_CRITICAL;
    } else if (b->crit == 2) {
        o->hitFx |= HITFX_WEAK;
    }
    o->hitSe = BowgunShot_KindUnk06B[b->stage];
    if (b->power != 0.0f) {
        v = b->power * BowgunShot_KindScale[b->stage];
        if (!(v < 255.0f)) {
            o->power = 255;
        } else if (v <= 1.0f) {
            o->power = 1;
        } else {
            o->power = v;
        }
    }
}

void BowgunShot_DrawStageModel(ObjAttach *o)
{
    f32 m[16];
    Model *mdl = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *model;
    s16 i;

    if (mdl != NULL && mdl->active != 0) {
        model = &mdl->matSets[89];
        Gs_SetState(0x60, 0x80);
        tex = mdl->mats;
        for (i = 0; i < 2; i++) {
            Mtx_Identity(m);
            Mtx_RotateX(m, (6.2831855f * o->rotX) / 65536.0f);
            Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            if (i != 0) {
                Mtx_TurnZ(m, 3.1415927f);
            }
            Gs_SetState(0x1A, (s32) m);
            if (model != NULL && model->id != -1) {
                Gs_SetLightList(tex, model);
                Draw_ApplyStates(model->flags);
                Model_Draw(model->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

u16 BowgunShot_MulAddShift(u16 randBits, u16 arg1, s32 arg2, s32 shift)
{
    return ((s32) ((randBits & 0xFFFF) + ((arg1 & 0xFFFF) * (arg2 & 0xFFFF))) >> (shift & 0xFFFF)) & 0xFFFF;
}

f32 BowgunShot_BitsToFrac(u16 randBits, s32 bits)
{
    s32 shift;

    shift = (0x10 - (bits & 0xFFFF)) & 0xFFFF;
    return 0.000015625f * (f32) ((randBits & 0xFFFF & ((0xFFFF >> shift) & 0xFFFF)) << shift);
}

f32 BowgunShot_BitsToSignedFrac(u16 value, u16 bits)
{
    u16 shift = 16 - bits;

    return 0.000015625f * (s16) (((value & (u16) (0xFFFF >> shift)) << shift) - 0x8000);
}

void BowgunShot_BoneLaunchPos(f32 *out, Player *p, s32 *ang, s16 bone)
{
    f32 v[3];
    f32 m[16];
    f32 ax;
    f32 ay;

    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(p, bone)));
    Mtx_GetTrans(out, m);
    ax = Math_Atan2(m[9], Math_Sqrtf(m[8] * m[8] + m[10] * m[10]));
    ay = 3.1415927f + Math_Atan2(m[8], m[10]);
    ang[0] = (u16) (s32) (0.5f + (65536.0f * ax) / 6.2831855f);
    ang[1] = (u16) (s32) (0.5f + (65536.0f * ay) / 6.2831855f);
    v[0] = -10.0f;
    v[1] = -10.0f;
    v[2] = 87.0f;
    Vec_RotateX(v, ax);
    Vec_RotateY(v, ay);
    out[0] += v[0];
    out[1] += v[1];
    out[2] += v[2];
}

void BowgunShot_PlaySe(ObjAttach *o, s16 kind)
{
    s16 s;

    if (o->area == gScene.area) {
        switch (kind) {
        case 0:
            switch (o->type) {
            case 12:
            case 13:
            case 14:
                Sound_PlaySeAtPosChecked(1, 0x4E, 0, o->pos, 1, 0);
                break;
            }
            break;
        case 1:
            if (o->type == 32) {
                break;
            }
            if (LightBowgun_HasSilencer(o->owner)) {
                if (((Player *) o->owner)->weaponType == WEAPON_HEAVY_BOWGUN) {
                    s = 5;
                } else {
                    s = 6;
                }
            } else {
                s = 0;
            }
            switch (Bowgun_RecoilLevel(o->owner, o->type)) {
            case 0:
                Player_PlaySe(o->owner, s + 10, 0, o->pos, 1, 0);
                break;
            case 1:
                Player_PlaySe(o->owner, s + 11, 0, o->pos, 1, 0);
                break;
            case 2:
                Player_PlaySe(o->owner, s + 12, 0, o->pos, 1, 0);
                break;
            }
            break;
        case 2:
            Sound_PlaySeAtPosChecked(1, 0x96, 0, o->pos, 6, 0);
            break;
        }
    }
}

void BowgunShot_PartAlloc(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    BowgunShotPart *p;

    buf->partTimer = 0;
    p = &buf->part;
    p->drawSlot = DrawSlot_Alloc();
    if (p->drawSlot != -1) {
        p->timer = 0;
        p->unk0A = 0;
        p->unk06 = 0;
        p->slot = DrawSlot_Get(p->drawSlot);
        p->slot->owner = o;
        p->slot->part = 0;
        p->slot->draw = BowgunShot_PartDraw;
    } else {
        p->slot = NULL;
    }
    BowgunShot_PartRun(o);
}

void BowgunShot_PartRun(ObjAttach *o)
{
    BowgunShotBuf *buf = (BowgunShotBuf *) o->buf;
    BowgunShotPart *p = &buf->part;
    f32 v[3];

    Vec_Copy(v, &o->velX);
    Vec_Normalize(v);
    v[0] *= -25.0f;
    v[1] *= -25.0f;
    v[2] *= -25.0f;
    switch (buf->stage) {
    case 1:
        if (++buf->partTimer == 1 && o->area == gScene.area) {
            StageObj_K12_CreateAt(o->pos, 3, 0);
        }
        break;
    }
    (p->timer)++;
    if (p->slot != NULL) {
        p->slot->pos[0] = o->pos[0] + v[0];
        p->slot->pos[1] = o->pos[1] + v[1];
        p->slot->pos[2] = o->pos[2] + v[2];
        OT_AddByDepth(gDrawOtSorted, p->slot, 0x40U, 0);
    }
}

void BowgunShot_PartFree(ObjAttach *o)
{
    BowgunShotPart *p = &((BowgunShotBuf *) o->buf)->part;

    if (p->slot != NULL) {
        DrawSlot_Free(p->drawSlot);
    }
}

void BowgunShot_PartDraw(DrawSlot *d)
{
    f32 m[16];
    f32 m1[16];
    ObjAttach *o;
    BowgunShotPart *e;
    ModelMat *tex;
    s16 k;
    f32 *c;
    BowgunShotBuf *buf;
    Model *mdl;
    ModelMatSet *model;
    s16 i;
    f32 sx;
    f32 sy;
    f32 sz;

    o = d->owner;
    mdl = gCommonModels[0];
    buf = (BowgunShotBuf *) o->buf;
    e = &buf->part + d->part;
    if (o->area == gScene.area && mdl != NULL && mdl->active != 0 && buf->stage <= 2) {
        tex = mdl->mats;
        k = e->timer & 1;
        for (i = 0; i < 2; i++) {
            if (i == 0) {
                model = &mdl->matSets[64];
                sx = BowgunShot_PartScaleA[k][0];
                sy = BowgunShot_PartScaleA[k][1];
                sz = BowgunShot_PartScaleA[k][2];
            } else {
                model = &mdl->matSets[66];
                if (buf->stage == 1 && buf->partTimer <= 5) {
                    c = BowgunShot_PartColors[buf->partTimer];
                    sx = c[0];
                    sy = c[1];
                    sz = c[2];
                } else {
                    sx = BowgunShot_PartScaleB[k][0];
                    sy = BowgunShot_PartScaleB[k][1];
                    sz = BowgunShot_PartScaleB[k][2];
                }
                Mtx_Translation(m1, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m1);
            }
            Mtx_Scale(m, sx, sy, sz);
            Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
            Mtx_Concat3x3(m, gCamMatrix);
            if (model != NULL && model->id != -1) {
                Gs_SetState(0x60, 0);
                Gs_SetState(0x1A, (s32) m);
                Gs_SetState(0x67, -0x41);
                Gs_SetLightList(tex, model);
                Draw_ApplyStates(model->flags);
                Gfx_SetBlend(4, 1);
                Model_Draw(model->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_EM_IMPACT_ATTACK;
            o->type = kind;
            o->update = EmImpactAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            switch (o->type) {
            case 5:
            case 6:
                o->hitScale = em->scaleX;
                break;
            }
        }
    }
}

void EmImpactAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        EmImpactAttack_Setup(o);
        break;
    case ATTACH_RUN:
        EmImpactAttack_Run(o);
        break;
    case ATTACH_END:
        EmImpactAttack_End(o);
        break;
    case ATTACH_HIT:
        EmImpactAttack_Run(o);
        break;
    case 4:
        EmImpactAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        EmImpactAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmImpactAttack_End(o);
        break;
    case 7:
        EmImpactAttack_End(o);
        break;
    case ATTACH_FREE:
        EmImpactAttack_Free(o);
        
    default:
        break;
    }
}

void EmImpactAttack_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 6:
        Player_TestSetFlags(o, ATKF_NO_PLAYER);
        break;
    case 7:
        Player_TestSetFlags(o, ATKF_PIERCES);
        break;
    default:
        Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EmImpactAttack_RowsPtr);
    o->hitList = EmImpactAttack_HitShapes[o->hitListNo];
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

void EmImpactAttack_Run(ObjAttach *o)
{

    if (o->hitPhase == 0) {
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EmImpactAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EmImpactAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* unk_6C7AE0's .data (0x89BF88-0x89C6F0) */
s16 BowgunShot_PowerTbls_0[6] = {
#include "game/0089BF88.inc"
};
s16 BowgunShot_PowerTbls_1[6] = {
#include "game/0089BF98.inc"
};
s16 BowgunShot_PowerTbls_2[6] = {
#include "game/0089BFA8.inc"
};
s16 BowgunShot_PowerTbls_3[6] = {
#include "game/0089BFB8.inc"
};
s16 BowgunShot_PowerTbls_4[6] = {
#include "game/0089BFC8.inc"
};
s16 BowgunShot_PowerTbls_5[6] = {
#include "game/0089BFD8.inc"
};
s16 BowgunShot_PowerTbls_6[4] = {
#include "game/0089BFE8.inc"
};
s16 BowgunShot_PowerTbls_7[8] = {
#include "game/0089BFF0.inc"
};
s16 BowgunShot_PowerTbls_8[8] = {
#include "game/0089C000.inc"
};
s16 BowgunShot_PowerTbls_9[8] = {
#include "game/0089C010.inc"
};
s16 BowgunShot_PowerTbls_10[4] = {
#include "game/0089C020.inc"
};
s16 BowgunShot_PowerTbls_11[4] = {
#include "game/0089C028.inc"
};
s16 BowgunShot_PowerTbls_12[4] = {
#include "game/0089C030.inc"
};
s16 BowgunShot_PowerTbls_13[4] = {
#include "game/0089C038.inc"
};
s16 BowgunShot_PowerTbls_14[4] = {
#include "game/0089C040.inc"
};
s16 BowgunShot_PowerTbls_15[4] = {
#include "game/0089C048.inc"
};
s16 BowgunShot_PowerTbls_16[4] = {
#include "game/0089C050.inc"
};
s16 BowgunShot_PowerTbls_17[4] = {
#include "game/0089C058.inc"
};
s16 *BowgunShot_PowerTbls[18] = {
    BowgunShot_PowerTbls_0, BowgunShot_PowerTbls_1, BowgunShot_PowerTbls_2, BowgunShot_PowerTbls_3,
    BowgunShot_PowerTbls_4, BowgunShot_PowerTbls_5, BowgunShot_PowerTbls_6, BowgunShot_PowerTbls_7,
    BowgunShot_PowerTbls_8, BowgunShot_PowerTbls_9, BowgunShot_PowerTbls_10, BowgunShot_PowerTbls_11,
    BowgunShot_PowerTbls_12, BowgunShot_PowerTbls_13, BowgunShot_PowerTbls_14, BowgunShot_PowerTbls_15,
    BowgunShot_PowerTbls_16, BowgunShot_PowerTbls_17,
};
u16 BowgunShot_LifeLimit[4] = {
#include "game/0089C0A8.inc"
};
s32 BowgunShot_ScatterParams[4] = {
#include "game/0089C0B0.inc"
};
s32 BowgunShot_BlendStates[4] = {
#include "game/0089C0C0.inc"
};
s16 BowgunShot_Kind15Tbl[8] = {
#include "game/0089C0D0.inc"
};
s16 BowgunShot_Kind16Tbl[8] = {
#include "game/0089C0E0.inc"
};
f32 BowgunShot_KindScale[6] = {
#include "game/0089C0F0.inc"
};
s16 BowgunShot_KindUnk06B[6] = {
#include "game/0089C108.inc"
};
f32 BowgunShot_PartScaleA[2][3] = {
#include "game/0089C120.inc"
};
f32 BowgunShot_PartScaleB[2][3] = {
#include "game/0089C140.inc"
};
f32 BowgunShot_PartColors[6][3] = {
#include "game/0089C160.inc"
};
HitShape EmImpactAttack_HitShapes_0[2] = {
#include "game/0089C1B0.inc"
};
HitShape EmImpactAttack_HitShapes_1[2] = {
#include "game/0089C200.inc"
};
HitShape EmImpactAttack_HitShapes_2[2] = {
#include "game/0089C250.inc"
};
HitShape EmImpactAttack_HitShapes_3[2] = {
#include "game/0089C2A0.inc"
};
HitShape EmImpactAttack_HitShapes_4[2] = {
#include "game/0089C2F0.inc"
};
HitShape EmImpactAttack_HitShapes_5[2] = {
#include "game/0089C340.inc"
};
HitShape EmImpactAttack_HitShapes_6[2] = {
#include "game/0089C390.inc"
};
HitShape EmImpactAttack_HitShapes_7[2] = {
#include "game/0089C3E0.inc"
};
HitShape EmImpactAttack_HitShapes_8[2] = {
#include "game/0089C430.inc"
};
HitShape EmImpactAttack_HitShapes_9[2] = {
#include "game/0089C480.inc"
};
HitShape EmImpactAttack_HitShapes_10[2] = {
#include "game/0089C4D0.inc"
};
HitShape *EmImpactAttack_HitShapes[12] = {
    EmImpactAttack_HitShapes_0, EmImpactAttack_HitShapes_1, EmImpactAttack_HitShapes_2, EmImpactAttack_HitShapes_3,
    EmImpactAttack_HitShapes_4, EmImpactAttack_HitShapes_5, EmImpactAttack_HitShapes_6, EmImpactAttack_HitShapes_7,
    EmImpactAttack_HitShapes_8, EmImpactAttack_HitShapes_9, EmImpactAttack_HitShapes_10, 0,
};
u32 EmImpactAttack_Rows[102] = {
#include "game/0089C550.inc"
};
void *EmImpactAttack_RowsPtr = (void *)EmImpactAttack_Rows;
