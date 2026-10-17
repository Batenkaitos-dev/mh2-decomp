/*
 * Game overlay code at 0x6F8A10-0x6FF630, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "area.h"
#include "objattach.h"

extern EmObj *gSightBlockObjs[32];

/* 8 bytes per entry of the per-kind tables at gEmReactions. */
typedef struct EmReaction {
    s32 unk0;
    s32 unk4;
} EmReaction;

/* Per-kind rates (gEmRateTbl, gEmDecay7ECRates, D_008BC7D0). */
typedef struct EmRate {
    s32 unk00[2];
    s32 unk08;          /* 0x08 */
    s32 unk0C;          /* 0x0C */
    s32 unk10;          /* 0x10 */
    s32 unk14;          /* 0x14 */
} EmRate;

extern EmRate *gEmRateTbl[];

/* Em +0x6C4: four s16 values, one per player, and +0x6CC their last seen copies (Em_UpdateReactions runs
 * the reaction of a player whose value changed). Em has no fields there yet: a view of it. */
typedef struct EmReactView {
    u8 pad[0x6C4];
    s16 cur[4];
    s16 last[4];
} EmReactView;

/* Em +0x818: what Em_SensePlayers measures of each player each frame (em.h has bytes there: other
 * code uses +0x846 as an s16). */
typedef struct EmSenseView {
    u8 pad[0x7E4];
    s16 unk7E4[4];              /* 0x7E4: Em unk7E4 */
    u8 pad7EC[0x818 - 0x7EC];
    f32 dist[4];                /* 0x818: from the monster to the player */
    f32 distXZ[4];              /* 0x828: the same, in the ground plane */
    f32 range[4];               /* 0x838: the four reaction ranges (Em_GetReaction kinds 3 to 6) */
    s32 unk848[4];              /* 0x848: Em unk848, cleared when the player is out of reach */
    u16 dir[4];                 /* 0x858: the direction to the player (Math_AngleXZ) */
} EmSenseView;

/* prototypes */
void AreaObj_K0E_Draw(DrawSlot *ds);
extern f32 AreaObj_K14_SePos[];
extern f32 D_008B149C[3];
void StageShot_CreateAt(f32 *pos, u8 kind, u8 area, s32 ang);
void CamShake_SetA(s32 kind, f32 *pos);
extern s16 AreaObj_K17_AngleTbl[];
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
extern EmMotionRow *gEmMotionRows[];
extern EmMotionRow *gEmMotionAltRows[];
extern EmMotionRow *gEmMotionTransRows[];
extern f32 *gEmSenseLimits[];
s8 Em_AngleSector(Em *em, s32 idx);
void Em_AddAlertBuildup(Em *em, u8 player, u8 idx, s8 kind);
extern s32 *gEmNoticeDecayRates[];
s32 EmBlangonga_CarriedShown(Em *em, u16 part);
s32 Em_PartBroken(Em *em, s32 arg1);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
extern f32 AreaObj_K0B_SePos[];
extern f32 D_008B0B8C[];
extern f32 AreaObj_K0B_EffectPos[][3];
extern EmObj *gFlashObjs[32];
u16 Math_AngleXZ(f32 *from, f32 *to);
typedef struct EmSight {
    f32 *pos;
    s32 unk04;
    s32 dir;
    u16 half;
} EmSight;
f32 Math_Cos(f32 x);
f32 Math_Absf(f32 x);
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
s32 Cyl_Contains(f32 *p, f32 *c, f32 r, f32 top, f32 bottom);
f32 Math_Sin(f32 x);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
extern s16 AreaObj_K0B_ModelsA[];
extern s16 AreaObj_K0B_ModelsB[];
extern u16 AreaObj_K0B_Angles[];
extern f32 AreaObj_K09_Type11Pos[];
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
extern f32 AreaObj_K0A_Offsets[];
extern f32 D_008B0AC4[];
extern f32 D_008B0AC8[];
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void CamShake_SetAEverywhere(s32);
extern f32 AreaObj_K14_PosA[][3];
extern f32 AreaObj_K14_PosB[][3];
u8 Player_InCurrentArea(void *obj);
void AreaObj_K14_Draw(DrawSlot *ds);
typedef struct ObjPart5 {
    s16 slot;
    s16 idx;
    f32 pos[3];
    u16 ang;
    u8 pad12[2];
    DrawSlot *ds;
} ObjPart5;
extern f32 AreaObj_K0B_Pos[][3];
extern f32 AreaObj_K0B_Rise[];
extern s16 AreaObj_K0B_Spin[];
void AreaObj_K0B_Draw(DrawSlot *s);
u8 Quest_GetSeedByte(u8 i);
void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void VolcanoHazard_Create(f32 *pos, u8 k, u8 c);
s32 AreaObj_K19_GetPoints(u16 arg0);
s16 AreaObj_K19_PointCount(u16 arg0);
extern f32 AreaObj_K09_Type8Pos[];
extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateY(f32 *v, f32 a);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 *AreaObj_K13_Params[];
void AreaObj_K13_Draw(DrawSlot *ds);
void AreaObj_K14_Update(Obj *o);
extern f32 gAreaObjK0A_PosX;
extern f32 D_008B0AAC;
extern f32 gAreaObjK0A_PosZ;
void AreaObj_K17_Draw(DrawSlot *slot);
void AreaObj_K0A_Draw(DrawSlot *ds);
void AreaObj_K16_Draw(DrawSlot *ds);
extern f32 AreaObj_K10_Pos[][3];
void Vec_Copy(f32 *dst, f32 *src);
void AreaObj_K10_Update(Obj *o);
void AreaObj_K10_Draw(DrawSlot *ds);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void AreaObj_K11_Draw(DrawSlot *s);
f32 *Mtx_FromAnglesXYZ(s32 *ang, f32 *mtx);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
void Mtx_TurnZ(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
extern s16 AreaObj_K10_Models[];
extern f32 AreaObj_K10_Scales[];
void Mtx_RotateY(f32 *m, f32 a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Em_BonePos(void *obj, s16 bone, f32 *out);
f32 Vec_Distance(f32 *a, f32 *b);
void Bone_GetAnimMoveFrom(f32 fparg0, f32 fparg1, f32 *out, EmBone *b);
void Vec_Blend(f32 *out, f32 *a, f32 *b, f32 wa, f32 wb);
f32 Math_Hermite(f32 x, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Identity(f32 *m);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void AreaObj_K09_DrawModel(ModelMatSet *set, s32 arg1);
extern f32 gCamMatrix[];
s32 Obj_SameArea(void *obj, Player *b);
struct Segment;
void Segment_Init(f32 *a, f32 *b, struct Segment *seg);
u8 Capsule_Classify(struct Segment *seg, f32 *p, f32 r);
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player);
void Em_AddNotice(Em *em, s32 a, s32 b, u8 player);
void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmDiablos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmDrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmGaruga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmGravios_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKirin_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKutku_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmLaoshan_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmRemobra_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmShenGaoren_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmYamaTsukami_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Obj_Free(Obj *p);
u16 Rand_Next(s32 stream);
struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);
void AreaObj_K09_ResetType8(Obj *o);
void AreaObj_K0A_Setup(Obj *o);
void AreaObj_K0A_Run(Obj *o);
void AreaObj_K0A_End(Obj *o);
void AreaObj_K0A_Free(Obj *o);
void AreaObj_K0B_Setup(Obj *o);
void AreaObj_K0B_Run(Obj *o);
void AreaObj_K0B_End(Obj *o);
void AreaObj_K0B_Free(Obj *o);
void AreaObj_K0E_Setup(Obj *o);
void AreaObj_K0E_Run(Obj *o);
void AreaObj_K0E_End(Obj *o);
void AreaObj_K0E_Free(Obj *o);
void AreaObj_K10_Setup(Obj *o);
void AreaObj_K10_Run(Obj *o);
void AreaObj_K10_End(Obj *o);
void AreaObj_K10_Free(Obj *o);
void AreaObj_K11_Setup(Obj *o);
u32 AreaObj_K11_Run(Obj *o);
void AreaObj_K11_End(Obj *o);
void AreaObj_K11_Free(Obj *o);
void AreaObj_K13_Setup(Obj *o);
u32 AreaObj_K13_Run(Obj *o);
void AreaObj_K13_End(Obj *o);
void AreaObj_K13_Free(Obj *o);
void AreaObj_K14_Setup(Obj *o);
void AreaObj_K14_Run(Obj *o);
void AreaObj_K14_End(Obj *o);
void AreaObj_K14_Free(Obj *o);
void AreaObj_K16_Setup(Obj *o);
void AreaObj_K16_Run(Obj *o);
void AreaObj_K16_End(Obj *o);
void AreaObj_K16_Free(Obj *o);
void AreaObj_K17_Setup(Obj *o);
void AreaObj_K17_Run(Obj *);
void AreaObj_K17_End(Obj *o);
void AreaObj_K17_Free(Obj *o);
void AreaObj_K19_Setup(Obj *o);
void AreaObj_K19_Run(Obj *o);
void AreaObj_K19_End(Obj *o);
void AreaObj_K19_Free(Obj *o);
void Em_TurnToward(Em *em, f32 *target, s32 mode);
void Em_SetState(Em *em, s32 group, s32 state);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ComputeGoal2(Em *em, f32 *target);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_AddNeed1(Em *em, s32 n);
void Em_UseGoalAction(Em *em);
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s32 Sprite_Draw(Sprite *s);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetTexFilter(s32 a);
void Tex_Bind(s32 i);
void DrawSlot_Free(s32 i);
extern u8 AreaObj_K19_PointsA2;
extern u8 AreaObj_K19_PointsA3;
extern u8 AreaObj_K19_PointsA6;
extern u8 AreaObj_K19_PointsA7;
extern u8 *gEmRows638[];
void Player_ForceState(void *obj, s32 state, s32 sub);
Obj *Obj_Alloc(s32 n);
void AreaObj_K0A_Update(Obj *o);
void AreaObj_K0B_Update(Obj *o);
void AreaObj_K0E_Update(Obj *o);
void AreaObj_K11_Update(Obj *o);
void AreaObj_K16_Update(Obj *o);
void AreaObj_K19_Update(Obj *o);
void AreaObj_K13_Update(Obj *o);
void AreaObj_K17_Update(Obj *o);
s32 Model_Draw(u32 h, s32 flags);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 reg, u32 v);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern EmAction *gAreaObjList[32];
u32 Player_TestFlag(void *obj, u32 flag);
f32 Em_LevelAttackFactor(Em *em);
f32 Em_LevelDamageFactor(Em *em);
extern EmRageRow *gEmRageTbl[];
extern s16 AreaObj_K14_Models[];
void Em_SyncState(Em *em, s32 kind, s32 b, s32 c);
s32 Game_IsOnline(void);
extern s32 *gEmPartBuildupLimits[];
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
u16 Area_DayId(u16 id);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
extern u8 AreaObj_K11_TileIdsA[];
extern u8 AreaObj_K11_TileIdsB[];
extern u16 AreaObj_K11_Tiles[];
extern f32 gViewMatrix[];
extern f32 gFrustumPlanes[];
void Camera_BuildFrustum(f32);
s32 Frustum_TestSphere(f32 *p, f32 *out, f32 *m, f32 *fr, f32 r);
typedef struct EmBendView {
    u8 pad[0x63C];
    u32 bend[4];
} EmBendView;
extern EmBendRow *gEmBendOrigins[];
/* end of prototypes */

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
u32 AreaObj_K09_RunType7(Obj *o)
{
    u8 *d = o->buf;
    f32 v[4];

    *(u16 *) (d + 0x4E) += (s32) *(u16 *) (d + 0x54);
    Mtx_GetTrans(v, (f32 *)d);
    Vec_RotateY(v, (6.2831855f * *(u16 *) (d + 0x4E)) / 65536.0f);
    o->ds->pos[0] = o->pos[0] + v[0];
    o->ds->pos[1] = o->pos[1] + v[1];
    o->ds->pos[2] = o->pos[2] + v[2];
    return OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

/* Draws the stage model of the area's entry +0x10 (when there is one) at the object, turned by the
 * object's heading (+0x4E of its data, in 1/65536 turns) and tilted by 10 degrees one way or the other
 * by o[7]; AREA_FOREST_HILLS_6 draws it three times bigger. The default-first switch leaves the game's empty
 * delay slots. */
void AreaObj_K09_DrawType7(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    f32 *d = o->buf;
    ModelMatSet *model;
    f32 m[16];
    s16 k;
    u16 area;

    if (stage != NULL) {
        if (stage->active == 0) {
            return;
        }
        area = gScene.area;
        k = gAreaAmbModels[area][8];
        if (k >= 0) {
            model = &stage->matSets[k];
            switch (area) {
            default:
                Mtx_Identity(m);
                break;
            case AREA_FOREST_HILLS_6:
                Mtx_Scale(m, 3.0f, 3.0f, 3.0f);
                break;
            }
            Mtx_RotateY(m, (6.2831855f * *(u16 *) ((u8 *) d + 0x4E)) / 65536.0f);
            Mtx_Concat3x3(m, d);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            if (o->var2 == 0) {
                Mtx_TurnZ(m, -0.17453294f);
            } else {
                Mtx_TurnZ(m, 0.17453294f);
            }
            AreaObj_K09_DrawModel(model, (s32) m);
        }
    }
}

void AreaObj_K09_ResetType8(Obj *o)
{

    if (o->var4 != 0xFF) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_9:
        case AREA_FOREST_HILLS_NIGHT_9:
            o->pos[0] = 11750.0f + (f32) ((Rand_Next(1) & 0x1FF) - 0x100);
            o->pos[1] = 200.0f;
            o->pos[2] = 9500.0f + (f32) ((Rand_Next(1) & 0x1FF) - 0x100);
            break;
        case AREA_TOWER_8:
            o->pos[0] = AreaObj_K09_Type8Pos[o->var4 * 3];
            o->pos[1] = D_008B0A14[o->var4 * 3];
            o->pos[2] = D_008B0A18[o->var4 * 3];
            break;
        }
    }
    o->timer = 0;
    o->var3 = ((Rand_Next(1) & 3) + 5) * 60;
    o->var1 = Rand_Next(1) & 1;
    o->var2 = Rand_Next(1) & 1;
    o->fvar = 1.0f;
}

void AreaObj_K09_SetupType8(Obj *o)
{
    AreaObj_K09_ResetType8(o);
}

void AreaObj_K09_RunType8(Obj *o)
{
    f32 x;
    f32 z;

    switch (o->var0) {
    case 0:
        o->timer++;
        if (o->timer >= o->var3) {
            o->var0++;
            o->timer = 10;
        } else {
            o->fvar = Math_Absf(Math_Cos((6.2831855f * (o->timer << 10)) / 65536.0f));
        }
        x = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        z = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        if (o->var1) {
            x *= -1.0f;
        }
        if (o->var2) {
            z *= -1.0f;
        }
        o->pos[0] += x;
        o->pos[2] += z;
        o->pos[1] += (x + z) / 2.0f;
        break;
    case 1:
        if (--o->timer <= 0) {
            if (o->var4 == 0xFF) {
                o->state++;
                return;
            }
            o->var0 = 0;
            AreaObj_K09_ResetType8(o);
        } else {
            o->fvar -= o->fvar / 10.0f;
        }
        break;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
}

void AreaObj_K09_DrawType8(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 m[16];
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = gAreaAmbModels[gScene.area][9];
        if (k >= 0) {
            model = &stage->matSets[k];
            Gfx_SetBlend(4, 1);
            Gs_SetState(0x60, 0);
            Gs_SetState(0x6C, 0);
            if (gScene.area == AREA_TOWER_8) {
                Mtx_Identity(m);
            } else {
                Mtx_Scale(m, o->fvar, o->fvar, o->fvar);
            }
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            Mtx_Concat3x3(m, gCamMatrix);
            Gs_SetState(0x67, ((u8) (u32) (255.0f * o->fvar) << 24) | 0xFFFFFF);
            AreaObj_K09_DrawModel(model, (s32) m);
            Gfx_SetBlend(4, 5);
            Gs_SetState(0x6C, 1);
        }
    }
}

void AreaObj_K09_SetupType9(void)
{
}

void AreaObj_K09_RunType9(void)
{
}

void AreaObj_K09_DrawType9(void)
{
}

void AreaObj_K09_SetupType10(void)
{
}

void AreaObj_K09_RunType10(void)
{
}

void AreaObj_K09_DrawType10(void)
{
}

//---
void AreaObj_K09_SetupType11(Obj *o)
{

    o->var0 = Rand_Next(1) % o->var3;
    o->var1 = Rand_Next(1) % (o->var3 - 1) + 1;
    o->var1 = o->var1 + o->var0;
    if (o->var1 >= o->var3) {
        o->var1 -= o->var3;
    }
    o->fvar = 25.0f * (0.5f + 0.001f * (Rand_Next(1) & 0x3FF));
    o->var2 = 0;
    o->timer = Rand_Next(1);
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        o->pos[0] = AreaObj_K09_Type11Pos[o->var0 * 3];
        o->pos[1] = D_008B0A94[o->var0 * 3] + (f32) ((u8) Rand_Next(1) - 0x80);
        o->pos[2] = D_008B0A98[o->var0 * 3];
        o->var3 = Math_Heading(AreaObj_K09_Type11Pos[o->var1 * 3], D_008B0A98[o->var1 * 3], o->pos[0], o->pos[2]) + 0x4000;
        break;
    }
    o->var0 = (Rand_Next(1) & 0x1F) + 0x30;
}

void AreaObj_K09_RunType11(Obj *o)
{
    f32 v[4];
    f32 dst[4];
    f32 *vy;
    f32 *vz;
    f32 s;
    f32 h;

    o->timer++;
    o->var2++;
    o->var2 &= 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        dst[0] = AreaObj_K09_Type11Pos[o->var1 * 3];
        dst[1] = D_008B0A94[o->var1 * 3];
        dst[2] = D_008B0A98[o->var1 * 3];
        break;
    }
    if (Cyl_Contains(o->pos, dst, 100.0f, -1.0f, -1.0f) != 0) {
        o->state++;
        return;
    }
    v[0] = 0.0f;
    vy = &v[1];
    vz = &v[2];
    *vy = 0.0f;
    *vz = 10.0f;
    Vec_RotateY(v, (6.2831855f * o->var3) / 65536.0f);
    o->pos[0] += v[0];
    o->pos[1] += *vy;
    o->pos[2] += *vz;
    s = Math_Sin((6.2831855f * (360.0f * ((f32) o->timer / (f32) (o->var0 * 2)))) / 360.0f);
    h = o->fvar * s;
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1] + h;
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K09_DrawType11(DrawSlot *s)
{
    Obj *o = s->owner;
    Model *stage = gAreaSubModels[0];
    f32 m[16];
    ModelMatSet *mdl;
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = o->var2;
        mdl = stage->matSets + k + 8;
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(m, s->pos[0], s->pos[1], s->pos[2]);
        Mtx_RotateY(m, (6.2831855f * o->var3) / 65536.0f);
        AreaObj_K09_DrawModel(mdl, (s32) m);
    }
}

void AreaObj_K0A_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0xA;
        obj->type = 0;
        obj->update = AreaObj_K0A_Update;
    }
}

void AreaObj_K0A_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K0A_Setup(o);
        break;
    case 1:
        AreaObj_K0A_Run(o);
        break;
    case 2:
        AreaObj_K0A_End(o);
        break;
    case 3:
        AreaObj_K0A_Free(o);
        break;
    }
}

void AreaObj_K0A_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    o->pos[0] = gAreaObjK0A_PosX;
    o->pos[1] = D_008B0AAC;
    o->pos[2] = gAreaObjK0A_PosZ;
    o->timer = 0;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K0A_Draw;
        AreaObj_K0A_Run(o);
    } else {
        Obj_Free(o);
    }
}

void AreaObj_K0A_Run(Obj *o)
{
    Player *p = &gPlayers[gScene.localPlayer];
    f32 v[4];
    s16 k;

    switch (o->var0) {
    case 0:
        if (--o->timer <= 0) {
            o->var0++;
            o->timer = 16;
            CamShake_SetAEverywhere(0);
            Sound_PlaySeAtPosChecked(7, 0x60, 0, p->pos, 0x13, 0);
        }
        break;
    case 1:
        if (--o->timer <= 0) {
            o->var0 = 0;
            o->timer = (Rand_Next(1) & 0x1F) + 0x3C;
            o->pos[1] = D_008B0AAC;
            break;
        }
        if (o->timer == 0xD) {
            k = Rand_Next(1) & 3;
            v[0] = p->pos[0] + AreaObj_K0A_Offsets[k * 3];
            v[1] = p->pos[1] + D_008B0AC4[k * 3];
            v[2] = p->pos[2] + D_008B0AC8[k * 3];
            Sound_PlaySeAtPosChecked(7, 0x61, 0, v, 0x13, 0);
        }
        o->pos[1] -= 62.5f;
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt32, o->ds, 0x20, 1);
        break;
    }
}

void AreaObj_K0A_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K0A_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K0A_Draw(DrawSlot *ds)
{
    f32 mtx[16];
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    if ((subs != NULL) && (subs->active != 0)) {
        model = subs->matSets;
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(mtx, ds->pos[0], ds->pos[1], ds->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        if (model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void AreaObj_K0B_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0xB;
        obj->type = 0;
        obj->update = AreaObj_K0B_Update;
    }
}

void AreaObj_K0B_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K0B_Setup(o);
        break;
    case 1:
        AreaObj_K0B_Run(o);
        break;
    case 2:
        AreaObj_K0B_End(o);
        break;
    case 3:
        AreaObj_K0B_Free(o);
        break;
    }
}

//---
void AreaObj_K0B_Setup(Obj *o)
{
    ObjPart5 *r = o->buf;
    s16 i;

    o->state++;
    if (gScene.emEventFlags & 1) {
        o->var0 = 2;
    } else {
        o->var0 = 0;
    }
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    o->timer = 0;
    for (i = 0; i < 5; i++, r++) {
        r->idx = i;
        r->ang = 0;
        Vec_Copy(r->pos, AreaObj_K0B_Pos[i]);
        if (o->var0 == 2) {
            r->pos[1] += AreaObj_K0B_Rise[i];
            r->ang += (s32) (65536.0f * AreaObj_K0B_Spin[i] / 360.0f + 0.5f);
        }
        r->slot = DrawSlot_Alloc();
        if (r->slot != -1) {
            r->ds = DrawSlot_Get(r->slot);
            r->ds->owner = o;
            r->ds->part = i;
            r->ds->draw = AreaObj_K0B_Draw;
        } else {
            r->ds = NULL;
        }
    }
    AreaObj_K0B_Run(o);
}

//---
void AreaObj_K0B_Run(Obj *o)
{
    ObjPart5 *r = o->buf;
    Em *m = gEms;
    s16 st = o->var0;
    s16 i;

    switch (o->var0) {
    case 0:
        if (gScene.emEventFlags & 1) {
            for (i = 0; i < 20; i++, m++) {
                if (m->active != 0 && m->visible != 0 && Player_InCurrentArea(m) != 0
                    && (m->type == 7 || m->type == 0x32 || m->type == 0x37)) {
                    Em_PlaySeChecked(m, 0x1A, 0, AreaObj_K0B_SePos, 8, 0);
                    Em_PlaySeChecked(m, 0x1B, 0, D_008B0B8C, 8, 0);
                    break;
                }
            }
            for (i = 0; i < 3; i++) {
                Effect0D_SpawnAt(AreaObj_K0B_EffectPos[i], 0, 25.0f);
            }
            o->var0++;
        }
        break;
    case 1:
        if (++o->timer >= 5) {
            o->var0++;
        }
        break;
    case 2:
        break;
    }
    for (i = 0; i < 5; i++, r++) {
        switch (st) {
        case 0:
            if (r->idx >= 4) {
                continue;
            }
            break;
        case 1:
            r->pos[1] += AreaObj_K0B_Rise[r->idx] / 5.0f;
            r->ang += (s32) (65536.0f * (AreaObj_K0B_Spin[r->idx] / 5) / 360.0f + 0.5f);
            break;
        case 2:
            if (r->idx < 4) {
                continue;
            }
            break;
        }
        if (r->ds != NULL) {
            r->ds->pos[0] = r->pos[0];
            r->ds->pos[1] = r->pos[1];
            r->ds->pos[2] = r->pos[2];
            OT_AddByDepth(gDrawOt32, r->ds, 0x20, 0);
        }
    }
}

void AreaObj_K0B_End(Obj *o)
{
    u8 *p = o->buf;
    s16 i;

    o->state++;
    o->visible = 0;
    for (i = 0; i < 5; i++) {
        if (*(s32 *) (p + 0x14) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x18;
    }
}

void AreaObj_K0B_Free(Obj *o)
{
    Obj_Free(o);
}

/* Draws part s->index of the five of AreaObj_K0B_Setup (a stage model per part, in AREA_FORTRESS_3 and AREA_FORTRESS_NIGHT_3). */
void AreaObj_K0B_Draw(DrawSlot *s)
{
    ObjPart5 *r = &((ObjPart5 *) ((Obj *) s->owner)->buf)[s->part];
    Model *stage = gAreaSubModels[0];
    f32 m[16];
    ModelMatSet *mdl;

    if (stage != NULL && stage->active != 0) {
        switch (gScene.area) {
        case AREA_FORTRESS_3:
            mdl = &stage->matSets[AreaObj_K0B_ModelsA[r->idx]];
            break;
        case AREA_FORTRESS_NIGHT_3:
            mdl = &stage->matSets[AreaObj_K0B_ModelsB[r->idx]];
            break;
        default:
            return;
        }
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(m, s->pos[0], s->pos[1], s->pos[2]);
        Mtx_RotateXYZ(m, (6.2831855f * r->ang) / 65536.0f, (6.2831855f * AreaObj_K0B_Angles[r->idx]) / 65536.0f, 0.0f);
        Gs_SetState(0x1A, (s32) m);
        if (mdl != NULL && mdl->id != 0) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x60, 0);
            Draw_SetDefaultState();
        }
    }
}

void AreaObj_K0E_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0xE;
        obj->type = 0;
        obj->update = AreaObj_K0E_Update;
    }
}

void AreaObj_K0E_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K0E_Setup(o);
        break;
    case 1:
        AreaObj_K0E_Run(o);
        break;
    case 2:
        AreaObj_K0E_End(o);
        break;
    case 3:
        AreaObj_K0E_Free(o);
        break;
    }
}

/* Set-up state of AreaObj_K0E_Update's kind: picks one of seven places by the area (Area_DayId of
 * gScene.area), with random frame and counter values; frees itself in other areas. */
void AreaObj_K0E_Setup(Obj *o)
{
    DrawSlot *ds;

    o->state++;
    o->visible = 1;
    o->var3 = 0;
    o->timer = 0;
    switch (Area_DayId(gScene.area)) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        o->type = 0;
        o->pos[0] = 7227.0f;
        o->pos[1] = -34.0f;
        o->pos[2] = 5795.0f;
        break;
    case AREA_DESERT_NIGHT_7:
        o->type = 1;
        o->pos[0] = 8800.0f;
        o->pos[1] = 13.0f;
        o->pos[2] = 10400.0f;
        o->var0 = Rand_Next(1) % 5;
        o->timer = Rand_Next(1) & 0xF;
        o->var2 = Rand_Next(1) % 10 + 55;
        o->var1 = Rand_Next(1) % 4 + 1;
        o->var1 += o->var0;
        if (o->var1 >= 5) {
            o->var1 -= 5;
        }
        o->var3 = Rand_Next(1) & 0xF;
        o->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        o->type = 2;
        o->pos[0] = 9400.0f;
        o->pos[1] = 18.0f;
        o->pos[2] = 9780.0f;
        o->var0 = Rand_Next(1) % 5;
        o->timer = Rand_Next(1) & 0xF;
        o->var2 = Rand_Next(1) % 10 + 55;
        o->var1 = Rand_Next(1) % 4 + 1;
        o->var1 += o->var0;
        if (o->var1 >= 5) {
            o->var1 -= 5;
        }
        o->var3 = Rand_Next(1) & 0xF;
        o->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_2:
        o->type = 3;
        o->pos[0] = 6200.0f;
        o->pos[1] = 54.0f;
        o->pos[2] = 5500.0f;
        o->var0 = Rand_Next(1) % 4;
        o->timer = Rand_Next(1) & 0xF;
        o->var2 = Rand_Next(1) % 10 + 55;
        o->var1 = Rand_Next(1) % 3 + 1;
        o->var1 += o->var0;
        if (o->var1 >= 4) {
            o->var1 -= 4;
        }
        o->var3 = Rand_Next(1) & 0xF;
        o->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_3:
        o->type = 4;
        o->pos[0] = 10000.0f;
        o->pos[1] = 13.0f;
        o->pos[2] = 10300.0f;
        o->var0 = Rand_Next(1) % 4;
        o->timer = Rand_Next(1) & 0xF;
        o->var2 = Rand_Next(1) % 10 + 55;
        o->var1 = Rand_Next(1) % 3 + 1;
        o->var1 += o->var0;
        if (o->var1 >= 4) {
            o->var1 -= 4;
        }
        o->var3 = Rand_Next(1) & 0xF;
        o->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_9:
    case AREA_DESERT_9:
        o->type = 5;
        o->pos[0] = 10000.0f;
        o->pos[1] = 13.0f;
        o->pos[2] = 10300.0f;
        o->var0 = Rand_Next(1) % 4;
        o->timer = Rand_Next(1) & 0xF;
        o->var2 = Rand_Next(1) % 10 + 55;
        o->var1 = Rand_Next(1) % 3 + 1;
        o->var1 += o->var0;
        if (o->var1 >= 4) {
            o->var1 -= 4;
        }
        o->var3 = Rand_Next(1) & 0xF;
        o->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        o->type = 6;
        o->var3 = 100;
        break;
    default:
        Obj_Free(o);
        return;
    }
    o->draw = NULL;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K0E_Draw;
        AreaObj_K0E_Run(o);
    } else {
        Obj_Free(o);
    }
}

/* Its run state: re-rolls the two counters (unk005, unk006) at random intervals (unk007, var4),
 * then draws unless unk014 (the draw callback) is set. */
void AreaObj_K0E_Run(Obj *o)
{

    switch (o->type) {
    case 1:
        if (++o->timer > o->var2) {
            o->var0 = Rand_Next(1) % 4 + 1;
            o->var0 += o->var1;
            if (o->var0 >= 5) {
                o->var0 -= 5;
            }
            o->timer = 0;
            o->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++o->var3 > o->var4) {
            o->var1 = Rand_Next(1) % 4 + 1;
            o->var1 += o->var0;
            if (o->var1 >= 5) {
                o->var1 -= 5;
            }
            o->var3 = 0;
            o->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 2:
        if (++o->timer > o->var2) {
            o->var0 = Rand_Next(1) % 4 + 1;
            o->var0 += o->var1;
            if (o->var0 >= 5) {
                o->var0 -= 5;
            }
            o->timer = 0;
            o->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++o->var3 > o->var4) {
            o->var1 = Rand_Next(1) % 4 + 1;
            o->var1 += o->var0;
            if (o->var1 >= 5) {
                o->var1 -= 5;
            }
            o->var3 = 0;
            o->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 3:
        if (++o->timer > o->var2) {
            o->var0 = Rand_Next(1) % 3 + 1;
            o->var0 += o->var1;
            if (o->var0 >= 4) {
                o->var0 -= 4;
            }
            o->timer = 0;
            o->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++o->var3 > o->var4) {
            o->var1 = Rand_Next(1) % 3 + 1;
            o->var1 += o->var0;
            if (o->var1 >= 4) {
                o->var1 -= 4;
            }
            o->var3 = 0;
            o->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 4:
        if (++o->timer > o->var2) {
            o->var0 = Rand_Next(1) % 3 + 1;
            o->var0 += o->var1;
            if (o->var0 >= 4) {
                o->var0 -= 4;
            }
            o->timer = 0;
            o->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++o->var3 > o->var4) {
            o->var1 = Rand_Next(1) % 3 + 1;
            o->var1 += o->var0;
            if (o->var1 >= 4) {
                o->var1 -= 4;
            }
            o->var3 = 0;
            o->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 5:
        if (++o->timer > o->var2) {
            o->var0 = Rand_Next(1) % 3 + 1;
            o->var0 += o->var1;
            if (o->var0 >= 4) {
                o->var0 -= 4;
            }
            o->timer = 0;
            o->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++o->var3 > o->var4) {
            o->var1 = Rand_Next(1) % 3 + 1;
            o->var1 += o->var0;
            if (o->var1 >= 4) {
                o->var1 -= 4;
            }
            o->var3 = 0;
            o->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 6:
        if (++o->timer >= o->var3) {
            o->timer = 0;
            o->var3 = 100;
        } else if (o->timer >= 100) {
            return;
        }
        break;
    default:
        o->timer++;
        break;
    }
    if (o->draw == NULL) {
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt8, o->ds, 8, 1);
    }
}

void AreaObj_K0E_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K0E_Free(Obj *o)
{
    Obj_Free(o);
}

extern f32 AreaObj_K0E_UV[16][2];
extern f32 AreaObj_K0E_Pos[4][3];
extern f32 AreaObj_K0E_Lights1[5][3];
extern f32 AreaObj_K0E_Lights2[5][3];
extern f32 AreaObj_K0E_Lights3[4][3];
extern f32 AreaObj_K0E_Lights4[4][3];
extern f32 AreaObj_K0E_Lights5[4][3];
extern s16 AreaObj_K0E_On0[4];
extern s16 AreaObj_K0E_On1[4];
extern s16 AreaObj_K0E_On2[4];
extern s16 AreaObj_K0E_On3[4];
extern s16 AreaObj_K0E_On4[4];
extern s16 AreaObj_K0E_On5[4];
extern s16 AreaObj_K0E_On6[8];
f32 Math_Floor(f32 x);

/* One light of the moving kinds (1-5): the i == 0 light follows unk005/unk008/unk007, the others
 * unk006/unk00A/var4; it crosses its row (tbl) in d frames at t, fading in and out over 20. */
#define K0E_ROW(tbl, off)                                    \
    if (i == 0) {                                            \
        d = o->var2;                                       \
        t = o->timer;                                       \
        row = tbl[o->var0];                                \
        w = t * (1000.0f / d);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    } else {                                                 \
        d = o->var4;                                    \
        t = o->var3;                                       \
        row = tbl[o->var1];                                \
        w = t * (1000.0f / d);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    }                                                        \
    if (t < 20) {                                            \
        a = (u8) (255.0f * (0.05f * t));                     \
    } else if (d - 20 < t) {                                 \
        a = (u8) (255.0f * ((f32) (d - t) / 20.0f));         \
    } else {                                                 \
        a = 0xFF;                                            \
    }                                                        \
    mdl = stage->matSets + off;                              \
    Gs_SetState(0x67, a << 24 | 0xFFFFFF);                   \
    break;

/* The lights of area object K0E: per type up to four lights (the 0/1 lists AreaObj_K0E_On0-DD0), each
 * with a scrolled texture (by area time for lights 0 and 1, by unk008 for 2 and 3) and, in types 1-5,
 * moving along a row of its table (K0E_ROW). */
void AreaObj_K0E_Draw(DrawSlot *ds)
{
    f32 m1[16];
    f32 m0[16];
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *at = &gAreaState;
    ModelMatSet *mdl;
    s16 *on;
    f32 *pos;
    s32 off;
    s16 i;
    f32 u;
    f32 v;
    f32 z;
    f32 x;
    f32 y;
    f32 *row;
    f32 w;
    s16 t;
    s16 d;
    u8 a;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        switch (o->type) {
        case 0: on = AreaObj_K0E_On0; break;
        case 1: on = AreaObj_K0E_On1; break;
        case 2: on = AreaObj_K0E_On2; break;
        case 3: on = AreaObj_K0E_On3; break;
        case 4: on = AreaObj_K0E_On4; break;
        case 5: on = AreaObj_K0E_On5; break;
        case 6: on = AreaObj_K0E_On6; break;
        default: return;
        }
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x60, 0);
        pos = AreaObj_K0E_Pos[0];
        for (i = 0, off = 0; i < 4; i++) {
            if (*on++ != 0) {
                u = 0.0f;
                v = u;
                switch (i) {
                case 0:
                    if (gScene.area == AREA_DESERT_10) {
                        v = 1.0f - at->frame / 100.0f;
                    } else {
                        v = 1.0f - 0.011111111f * at->frame;
                    }
                    break;
                case 1:
                    if (gScene.area == AREA_JUNGLE_NIGHT_3) {
                        v = 1.0f - 0.015625f * at->frame;
                    } else {
                        v = 1.0f - 0.03125f * at->frame;
                    }
                    break;
                case 2:
                    u = AreaObj_K0E_UV[(o->timer & 0x1E) >> 1][0];
                    v = AreaObj_K0E_UV[(o->timer & 0x1E) >> 1][1];
                    break;
                case 3:
                    u = 0.0033333334f * ((u16) o->timer % 300);
                    break;
                }
                u -= Math_Floor(u);
                v -= Math_Floor(v);
                Mtx_Translation(m0, u, v, 0.0f);
                Gs_SetState(0x19, (s32) m0);
                x = 0.0f;
                y = x;
                z = x;
                switch (o->type) {
                case 0:
                    x = pos[0];
                    y = pos[1];
                    z = pos[2];
                    mdl = (ModelMatSet *) ((u8 *) stage->matSets + off + 0x8C);
                    break;
                case 1:
                    K0E_ROW(AreaObj_K0E_Lights1, 0)
                case 2:
                    K0E_ROW(AreaObj_K0E_Lights2, 5)
                case 3:
                    K0E_ROW(AreaObj_K0E_Lights3, 3)
                case 4:
                    K0E_ROW(AreaObj_K0E_Lights4, 7)
                case 5:
                    K0E_ROW(AreaObj_K0E_Lights5, 3)
                case 6:
                    mdl = stage->matSets + 2;
                    break;
                }
                Mtx_Translation(m1, x, y, z);
                Gs_SetState(0x1A, (s32) m1);
                if (mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Gfx_SetTexFilter(1);
                    Model_Draw(mdl->id, 0);
                }
            }
            pos += 3;
            off += 0x8C;
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 1);
    }
}

void AreaObj_K10_Create(void)
{
    s16 i;
    Obj *obj;

    switch (gScene.area) {
    default:
        return;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        for (i = 0; i < 5; i++) {
            obj = Obj_Alloc(0);
            if (obj != NULL) {
                obj->kind = 0x10;
                obj->type = i;
                obj->update = AreaObj_K10_Update;
            }
        }
        break;
    }
}

void AreaObj_K10_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K10_Setup(o);
        break;
    case 1:
        AreaObj_K10_Run(o);
        break;
    case 2:
        AreaObj_K10_End(o);
        break;
    case 3:
        AreaObj_K10_Free(o);
        break;
    }
}

void AreaObj_K10_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->var3 = 0;
    o->pos[0] = 0.0f;
    o->pos[1] = 0.0f;
    o->pos[2] = 0.0f;
    o->timer = 0;
    Vec_Copy(o->pos, AreaObj_K10_Pos[o->type]);
    o->var3 = (Rand_Next(1) & 0x7F) + 200;
    o->draw = NULL;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K10_Draw;
        AreaObj_K10_Run(o);
    } else {
        Obj_Free(o);
    }
}

void AreaObj_K10_Run(Obj *o)
{
    o->timer++;
    if (o->timer >= o->var3) {
        o->timer = 0;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K10_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K10_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K10_Draw(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaState.model;
    f32 m[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = &stage->matSets[AreaObj_K10_Models[o->type]];
        Gs_SetLightList(stage->mats, model);
        Mtx_Scale(m, AreaObj_K10_Scales[o->type], AreaObj_K10_Scales[o->type], AreaObj_K10_Scales[o->type]);
        Mtx_RotateY(m, (6.2831855f * (360.0f * (1.0f - (f32) o->timer / (f32) o->var3))) / 360.0f);
        Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x60, 0x80);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void AreaObj_K11_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x11;
        obj->type = 0;
        obj->update = AreaObj_K11_Update;
    }
}

void AreaObj_K11_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K11_Setup(o);
        break;
    case 1:
        AreaObj_K11_Run(o);
        break;
    case 2:
        AreaObj_K11_End(o);
        break;
    case 3:
        AreaObj_K11_Free(o);
        break;
    }
}

void AreaObj_K11_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->var3 = 0;
    o->pos[0] = 0.0f;
    o->pos[1] = 0.0f;
    o->pos[2] = 0.0f;
    o->timer = 0;
    o->draw = NULL;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K11_Draw;
        AreaObj_K11_Run(o);
    } else {
        Obj_Free(o);
    }
}

u32 AreaObj_K11_Run(Obj *o)
{
    Player *p;

    p = &gPlayers[gScene.localPlayer];
    o->timer++;
    o->pos[0] = p->pos[0];
    o->pos[1] = 0.0f;
    o->pos[2] = p->pos[2];
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    return OT_AddByDepth(gDrawOt8, o->ds, 8, 1);
}

void AreaObj_K11_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K11_Free(Obj *o)
{
    Obj_Free(o);
}

/* Draws the tiled ground models of areas 2/0x6F (5x5) and 0x74 (18x7, each tile turned by
 * AreaObj_K11_Tiles): the tile ids in AreaObj_K11_TileIdsA/AreaObj_K11_TileIdsB (0xFF: none), frustum-culled.
 * Case 2's statement order is the game's: rows and cols set after the radius fill the scheduler's
 * free slot beside the radius's chain, so tbl's lui waits until the radius is stored. */
void AreaObj_K11_Draw(DrawSlot *s)
{
    f32 out[4];
    f32 m[16];
    f32 sph[8];
    s32 k;
    ModelMatSet *mdl;
    u16 day;
    s16 i;
    s16 j;
    s16 rows;
    Model *stage = gAreaSubModels[0];
    u8 id;
    s16 cols;
    u8 *tbl;
    f32 x0;
    f32 xstep;
    f32 zstep;
    f32 z;
    f32 x;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x60, 0x80);
        Gs_SetState(0x67, -1);
        day = Area_DayId(gScene.area);
        switch (day) {
        case 2:
        case 0x6F:
            z = 4000.0f;
            x0 = 8000.0f;
            zstep = 2000.0f;
            sph[4] = 2828.0f;
            xstep = 2000.0f;
            rows = 5;
            cols = 5;
            tbl = AreaObj_K11_TileIdsA;
            Camera_BuildFrustum(4000.0f);
            break;
        case 0x74:
            z = 5800.0f;
            x0 = 10100.0f;
            rows = 18;
            cols = 7;
            xstep = 500.0f;
            zstep = 1000.0f;
            sph[4] = 1414.0f;
            tbl = AreaObj_K11_TileIdsB;
            Camera_BuildFrustum(4000.0f);
            break;
        }
        sph[2] = 0.0f;
        Tex_Prepare(0x10, 0x128);
        for (i = 0; i < rows; i++) {
            x = x0;
            for (j = 0; j < cols; j++) {
                k = i * cols + j;
                id = tbl[(s16) k];
                if (id != 0xFF) {
                    sph[1] = x;
                    sph[3] = z;
                    if (Frustum_TestSphere(&sph[1], out, gViewMatrix, gFrustumPlanes, sph[4]) != 0) {
                        mdl = stage->matSets + (id & 0xF);
                        Mtx_Translation(m, x, 0.0f, z);
                        if (day == 0x74) {
                            Mtx_RotateY(m, (6.2831855f * AreaObj_K11_Tiles[k]) / 65536.0f);
                        }
                        Gs_SetState(0x1A, (s32) m);
                        Draw_ApplyStates(mdl->flags);
                        Model_Draw(mdl->id, 0);
                    }
                }
                x += xstep;
            }
            z += zstep;
        }
    }
}

void AreaObj_K13_Create(u8 a)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x13;
        obj->type = a;
        obj->update = AreaObj_K13_Update;
    }
}

void AreaObj_K13_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K13_Setup(o);
        break;
    case 1:
        AreaObj_K13_Run(o);
        break;
    case 2:
        AreaObj_K13_End(o);
        break;
    case 3:
        AreaObj_K13_Free(o);
        break;
    }
}

void AreaObj_K13_Setup(Obj *o)
{
    f32 *f;

    f = AreaObj_K13_Params[o->type];
    o->state++;
    o->visible = 1;
    o->pos[0] = *f++;
    o->pos[1] = *f++;
    o->pos[2] = *f;
    o->timer = 0;
    o->var3 = 0;
    o->var5 = 0;
    o->fvar = 0.0f;
    o->draw = NULL;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K13_Draw;
        Vec_Copy(o->ds->pos, o->pos);
        AreaObj_K13_Run(o);
    } else {
        Obj_Free(o);
    }
}

u32 AreaObj_K13_Run(Obj *o)
{

    o->timer++;
    switch (o->type) {
    case 0x19:
        o->var3++;
        if (o->var3 >= 0x380) {
            o->var3 = 0;
        }
        break;
    case 0x24:
        o->var5++;
        if (o->var5 >= 0x4B0) {
            o->var5 = 0;
        }
        o->var3++;
        if (o->var3 >= 0x258) {
            o->var3 = 0;
        }
        break;
    case 0x25:
        o->var3 += 0x2E;
        break;
    case 0x36:
        o->var3++;
        if (o->var3 >= 0x320) {
            o->var3 = 0;
        }
        break;
    case 0x37:
        o->var3++;
        if (o->var3 >= 0x320) {
            o->var3 = 0;
        }
        break;
    default:
        o->var3 += 0x5B;
        break;
    }
    return OT_AddByDepth(gDrawOt8, o->ds, 8, 1);
}

void AreaObj_K13_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K13_Free(Obj *o)
{
    Obj_Free(o);
}

s32 func_006FD010(Em *em, u8 type)
{
    switch (em->subType) {
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x36:
    case 0x37:
        return 1;
    }
    return 0;
}

extern s16 AreaObj_K13_Pos[56];
extern s16 *AreaObj_K13_Models[56];
extern f32 AreaObj_K13_Scroll[16][2];

/* The models of area object K13 (by type): type 0x11 first draws the other stage's model 8. Then up
 * to four models (the 0 entries of AreaObj_K13_Models[type]; AreaObj_K13_Pos[type] the first model, AreaObj_K13_Params[type]
 * the positions), each with a texture scroll by type (unk008, unk00A; AreaObj_K13_Scroll a 4x4 tile grid).
 * Level 4 (the 8 FPU register words of the two 0.0333f * 360 * unk00A lines differ at 3): the functions
 * AreaObj_K11_Create-AreaObj_K14_Draw all match at 4, AreaObj_K16_Draw differs there. */
#pragma optimization_level 4
void AreaObj_K13_Draw(DrawSlot *ds)
{
    f32 m[16];
    f32 m2[16];
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *at = &gAreaState;
    ModelMat *lights;
    ModelMatSet *mdl;
    s16 k;
    s16 *flags;
    f32 *pos;
    f32 x;
    f32 y;
    f32 u;
    f32 v;

    if (o->type == 0x11) {
        stage = at->model;
        lights = stage->mats;
        if (stage == NULL) {
            return;
        }
        if (stage->active == 0) {
            return;
        }
        Mtx_Identity(m);
        mdl = stage->matSets + 8;
        Gs_SetLightList(lights, mdl);
        Gs_SetState(0x1A, (s32) m);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
        stage = gAreaSubModels[0];
    }
    if (func_006FD010((Em *) o, o->type)) {
        stage = at->model;
        lights = stage->mats;
    }
    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x67, -1);
        pos = AreaObj_K13_Params[o->type];
        flags = AreaObj_K13_Models[o->type];
        k = AreaObj_K13_Pos[o->type];
        if (*flags++ == 0) {
            Mtx_Identity(m);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) o, o->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = 0.0f;
            y = x;
            switch (o->type) {
            case 0x14:
            case 0x2C:
                Gs_SetState(0x60, 0);
                x = (o->timer & 0x1FF) / 512.0f;
                break;
            case 0x1B:
            case 0x1F:
            case 0x34:
                Gs_SetState(0x60, 0);
                x = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x10:
                Gs_SetState(0x60, 0);
                break;
            case 0xC:
                Gs_SetState(0x60, 0);
                x = 0.125f * (o->timer & 7);
                y = 0.125f * ((o->timer >> 3) & 3);
                break;
            case 0:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x1D:
            case 0x35:
                Gs_SetState(0x60, 0);
                break;
            case 0x1E:
                Gs_SetState(0x60, 0);
                x = 1.0f - (o->timer & 0x1FF) / 512.0f;
                break;
            case 0xB:
                Gs_SetState(0x60, 0);
                y = 1.0f - (o->timer & 0x1FF) / 512.0f;
                break;
            default:
                if (o->type == 0x25 || o->type == 0x29) {
                    Gs_SetState(0x60, 0);
                } else {
                    Gs_SetState(0x60, 0x80);
                }
                x = 0.033333335f * ((360.0f * o->var3) / 65536.0f);
                x -= Math_Floor(x);
                Mtx_RotateY(m, (6.2831855f * o->var3) / 65536.0f);
                break;
            }
            Mtx_Translation(m2, x, y, 0.0f);
            Gs_SetState(0x19, (s32) m2);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(m, x, y, *pos++);
            Gs_SetState(0x1A, (s32) m);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(0x60, 0);
        if (*flags++ == 0) {
            Mtx_Identity(m);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) o, o->type)) {
                Gs_SetLightList(lights, mdl);
            }
            switch (o->type) {
            case 0:
            case 1:
            case 0x14:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x1D:
            case 0x2B:
            case 0x2C:
                Mtx_Translation(m2, (o->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0x31:
                Gs_SetState(0x6C, 0);
                Mtx_Translation(m2, (o->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0xC:
            case 0x1B:
                Mtx_Translation(m2, 0.0f, (o->timer & 0x7F) / 128.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 8:
            case 0x28:
                Mtx_Translation(m2, 0.0f, 1.0f - (o->timer & 0x1FF) / 512.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0xB:
                Mtx_Translation(m2, 1.0f - (o->timer & 0xF) / 16.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0x1E:
                Mtx_Translation(m2, 0.125f * (o->timer & 7), 0.125f * ((o->timer >> 3) & 3), 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0x1F:
            case 0x34:
                Mtx_Translation(m2, (o->timer & 0xFF) / 256.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0x24:
                Mtx_Translation(m2, (o->timer & 0x7FF) / 2048.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) m2);
                break;
            case 0x10:
            case 0x26:
            case 0x29:
                break;
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(m, x, y, *pos++);
            Gs_SetState(0x1A, (s32) m);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x6C, 1);
        }
        if (*flags++ == 0) {
            Mtx_Identity(m);
            u = 0.0f;
            v = u;
            switch (o->type) {
            case 4:
            case 7:
                v = (o->timer & 0xFF) / 256.0f;
                break;
            case 0xC:
            case 0x14:
            case 0x2A:
            case 0x2B:
            case 0x2E:
                u = (o->timer & 0x1FF) / 512.0f;
                break;
            case 0x13:
                u = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x10:
            case 0x1B:
                break;
            case 0x19:
                v = o->var3 / 896.0f;
                u = v;
                break;
            case 0x1C:
                v = (o->timer & 0x7F) / 128.0f;
                break;
            case 1:
            case 0xD:
            case 0x1F:
            case 0x2C:
                u = 0.125f * (o->timer & 7);
                v = 0.125f * ((o->timer >> 3) & 3);
                break;
            case 0x1E:
                u = (o->timer & 0x3F) / 64.0f;
                break;
            case 0x25:
                u = 0.033333335f * ((360.0f * o->var3) / 65536.0f);
                u -= Math_Floor(u);
                Mtx_RotateY(m, (6.2831855f * o->var3) / 65536.0f);
                v = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x12:
            case 0x1A:
            case 0x23:
            case 0x27:
            case 0x30:
                u = (o->timer & 0x1FF) / 512.0f;
                break;
            case 8:
                u = (o->timer & 0x1FF) / 512.0f;
                v = 1.0f - (o->timer & 0x7F) / 128.0f;
                break;
            case 0x28:
                Gs_SetState(0x6C, 0);
                u = (o->timer & 0x1FF) / 512.0f;
                v = 1.0f - (o->timer & 0x7F) / 128.0f;
                break;
            case 0x24:
                u = o->var5 / 1200.0f;
                break;
            case 9:
            case 0x26:
            case 0x29:
                v = (o->timer & 0x7FF) / 2048.0f;
                break;
            case 0x21:
            case 0x22:
                u = 1.0f - (o->timer & 0x3FF) / 1024.0f;
                break;
            case 0x2D:
                v = (o->timer & 0x1FF) / 512.0f;
                break;
            case 0x33:
                v = 1.0f - (o->timer & 0x1FF) / 512.0f;
                break;
            case 0xB:
            case 0x34:
                break;
            case 0x36:
            case 0x37:
                u = (o->timer & 0x3FF) / 1024.0f;
                break;
            case 0x20:
                v = (o->timer & 0x3FF) / 1024.0f;
                break;
            default:
                u = AreaObj_K13_Scroll[o->timer & 0xF][0];
                v = AreaObj_K13_Scroll[o->timer & 0xF][1];
                break;
            }
            Mtx_Translation(m2, u, v, 0.0f);
            Gs_SetState(0x19, (s32) m2);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) o, o->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(m, x, y, *pos++);
            Gs_SetState(0x1A, (s32) m);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(0x6C, 1);
        if (*flags++ == 0) {
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) o, o->type)) {
                Gs_SetLightList(lights, mdl);
            }
            Mtx_Identity(m);
            switch (o->type) {
            case 2:
            case 3:
            case 5:
            case 6:
            case 0xF:
            case 0x20:
            case 0x21:
                y = 0.0f;
                x = (o->timer & 0x3FF) / 1024.0f;
                break;
            case 8:
            case 0x15:
            case 0x28:
                y = 0.0f;
                x = (o->timer & 0x7F) / 128.0f;
                break;
            case 9:
            case 0xC:
            case 0x14:
            case 0x23:
            case 0x26:
            case 0x27:
            case 0x29:
            case 0x2A:
            case 0x2E:
            case 0x2F:
                x = 0.125f * (o->timer & 7);
                y = 0.125f * ((o->timer >> 3) & 3);
                break;
            case 0xB:
                Gs_SetState(0x6C, 0);
                y = 0.0f;
                x = (o->timer & 0x7F) / 128.0f;
                break;
            case 0x1F:
                y = 0.0f;
                x = (o->timer & 0x3F) / 64.0f;
                break;
            case 0x1E:
                y = 0.0f;
                x = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x33:
                Gs_SetState(0x6C, 0);
                y = 0.0f;
                x = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x11:
            case 0x1A:
            case 0x1B:
                y = 0.0f;
                x = (o->timer & 0x1FF) / 512.0f;
                break;
            case 1:
            case 0xD:
            case 0x2C:
                x = 0.0f;
                y = (o->timer & 0x7F) / 128.0f;
                break;
            case 0x22:
                x = 0.0f;
                y = (o->timer & 0x7FF) / 2048.0f;
                break;
            case 0:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x1D:
            case 0x30:
                x = 0.0f;
                y = (o->timer & 0xFF) / 256.0f;
                break;
            case 0x24:
                y = 0.0f;
                x = o->var3 / 600.0f;
                break;
            case 0x36:
                y = 0.0f;
                x = o->var3 / 800.0f;
                break;
            case 0x37:
                y = 0.0f;
                x = o->var3 / 800.0f;
                Gs_SetState(0x67, 0xFF303030);
                break;
            case 0xA:
            case 0x32:
                x = 0.0f;
                y = x;
                break;
            }
            Mtx_Translation(m2, x, y, 0.0f);
            Gs_SetState(0x19, (s32) m2);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(m, x, y, *pos++);
            Gs_SetState(0x1A, (s32) m);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x6C, 1);
        }
        Draw_SetDefaultState();
    }
}
#pragma optimization_level reset

/* Creates the kind-0x14 object; in the quests 0xBBD-0xBBF and 0xEA68 (or any but 0xBBC) with a quest
 * running, sets gScene.skipFlaggedTris (the flag kind 0x14 runs on). */
void AreaObj_K14_Create(u8 kind)
{
    Obj *obj;

    switch (gQuest.stage) {
    default:
        switch (gQuest.info->id) {
        case 0xBBC:
            gScene.skipFlaggedTris = 0;
            break;
        case 0xBBD:
        case 0xBBE:
        case 0xBBF:
        case 0xEA68:
        default:
            gScene.skipFlaggedTris = 1;
            break;
        }
        break;
    case 0:
        break;
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 0x14;
        obj->arg = NULL;
        obj->type = kind;
        obj->update = AreaObj_K14_Update;
    }
}

void AreaObj_K14_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K14_Setup(o);
        break;
    case 1:
        AreaObj_K14_Run(o);
        break;
    case 2:
        AreaObj_K14_End(o);
        break;
    case 3:
        AreaObj_K14_Free(o);
        break;
    }
}

//---
void AreaObj_K14_Setup(Obj *o)
{
    s16 i;
    Em *m;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->fvar = 0.0f;
    if (gScene.skipFlaggedTris != 0) {
        Vec_Copy(o->pos, AreaObj_K14_PosA[o->type]);
    } else {
        Vec_Copy(o->pos, AreaObj_K14_PosB[o->type]);
    }
    if (gScene.emEventFlags & 2) {
        m = gEms;
        for (i = 0; i < 20; i++, m++) {
            if (m->active != 0 && m->visible != 0 && Player_InCurrentArea(m) != 0) {
                if (m->type == 2 || m->type == 0x24 || m->type == 0x47) {
                    break;
                }
            }
        }
        if (i == 20 || m->motionId[0] != 0x403) {
            o->var0 = 4;
        } else {
            o->var0 = 2;
        }
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = AreaObj_K14_Draw;
        AreaObj_K14_Run(o);
    } else {
        Obj_Free(o);
    }
}

/* Kind 0x14's run state (AreaObj_K14_Update, state 1). In object 1, while gScene.skipFlaggedTris is set:
 * waits for a monster of kind 2, 0x24 or 0x47 in the box x 10900-13000, z 19700-20200, then drops
 * 1750 below its table height with bounces, and after 180 frames rises back to it. */
void AreaObj_K14_Run(Obj *o)
{
    Em *m = gEms;
    s16 i;
    f32 v[3];

    switch (o->type) {
    case 1:
        if (gScene.skipFlaggedTris == 0) {
            break;
        }
        switch (o->var0) {
        case 0:
            for (i = 0; i < 20; i++, m++) {
                if (m->active != 0 && m->visible != 0 && Player_InCurrentArea(m) != 0) {
                    if (m->type == 2 || m->type == 0x24 || m->type == 0x47) {
                        break;
                    }
                }
            }
            if (i == 20) {
                break;
            }
            if (!(gScene.emEventFlags & 2) && m->motionId[0] != 0x403) {
                if (m->stance != 0) {
                    break;
                }
                if (!(m->posX > 10900.0f && m->posX < 13000.0f && m->posZ > 19700.0f && m->posZ < 20200.0f)) {
                    break;
                }
            }
            Em_PlaySeChecked(m, 0x36, 0, AreaObj_K14_SePos, 9, 0);
            Em_PlaySeChecked(m, 0x37, 0, D_008B149C, 9, 0);
            o->var0++;
            o->fvar = -100.0f;
            v[0] = o->pos[0];
            v[1] = m->floorY;
            v[2] = o->pos[2];
            StageShot_CreateAt(v, STAGESHOT_AREA_DROP, m->area, 0);
            break;
        case 1:
            o->pos[1] += o->fvar;
            o->fvar += -20.0f;
            if (o->pos[1] <= AreaObj_K14_PosA[o->type][1] - 1750.0f) {
                o->pos[1] = AreaObj_K14_PosA[o->type][1] - 1750.0f;
                o->fvar *= -0.2f;
                o->var0++;
                CamShake_SetA(5, o->pos);
                o->timer = 0;
            }
            break;
        case 2:
            if (o->var1 < 3) {
                o->pos[1] += o->fvar;
                o->fvar += -20.0f;
                if (o->pos[1] <= AreaObj_K14_PosA[o->type][1] - 1750.0f) {
                    o->pos[1] = AreaObj_K14_PosA[o->type][1] - 1750.0f;
                    o->fvar *= -0.2f;
                    o->var1++;
                }
            }
            for (i = 0; i < 20; i++, m++) {
                if (m->active != 0 && m->visible != 0 && Player_InCurrentArea(m) != 0) {
                    if (m->type == 2 || m->type == 0x24 || m->type == 0x47) {
                        break;
                    }
                }
            }
            if (++o->timer <= 180) {
                break;
            }
            if (i == 20 || m->motionId[0] != 0x403) {
                o->var0++;
                o->timer = 0;
                for (i = 0; i < 20; i++, m++) {
                    if (m->active != 0 && m->visible != 0 && Player_InCurrentArea(m) != 0) {
                        if (m->type == 2 || m->type == 0x24 || m->type == 0x47) {
                            Em_PlaySeChecked(m, 0x38, 0, AreaObj_K14_SePos, 9, 0);
                            Em_PlaySeChecked(m, 0x39, 0, D_008B149C, 9, 0);
                            break;
                        }
                    }
                }
            }
            break;
        case 3:
            o->pos[1] += 29.166666f;
            if (o->pos[1] > AreaObj_K14_PosA[o->type][1]) {
                o->var0++;
                o->pos[1] = AreaObj_K14_PosA[o->type][1];
                o->timer = 0;
            }
            break;
        case 4:
            break;
        }
        break;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K14_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void AreaObj_K14_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K14_Draw(DrawSlot *ds)
{
    f32 mtx[16];
    s32 w;
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    w = (s32) ds->owner;
    if ((subs != NULL) && (subs->active != 0)) {
        Mtx_Translation((void *) mtx, *(f32 *)((u8 *)w + 0x20), *(f32 *)((u8 *)w + 0x24), *(f32 *)((u8 *)w + 0x28));
        Gs_SetState(0x1A, (s32) mtx);
        model = &subs->matSets[AreaObj_K14_Models[*(u8 *)((u8 *)w + 3)]];
        if ((model != NULL) && (model->id != -1)) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void AreaObj_K16_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x16;
        obj->update = AreaObj_K16_Update;
    }
}

void AreaObj_K16_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K16_Setup(o);
        break;
    case 1:
        AreaObj_K16_Run(o);
        break;
    case 2:
        AreaObj_K16_End(o);
        break;
    case 3:
        AreaObj_K16_Free(o);
        break;
    }
}

void AreaObj_K16_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->timer = Rand_Next(1) & 0xF;
    o->var3 = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = AreaObj_K16_Draw;
        AreaObj_K16_Run(o);
    } else {
        Obj_Free(o);
    }
}

/* A background object (kind 0x16, AreaObj_K16_Create): after a random wait it reappears at one of six
 * placements (unk006) 21700 from (10000, 10000) at a random angle; placement 5 also spawns four
 * effects. y is a variable (the game adds 0.0f to v[1]). */
void AreaObj_K16_Run(Obj *o)
{
    f32 v[3];
    f32 y = 0.0f;

    switch (o->var0) {
    case 0:
        if (--o->timer >= 0) {
            break;
        }
        o->var0++;
        o->var1 = Rand_Next(1) % 5;
        o->timer = 7;
        v[0] = 0.0f;
        v[2] = 21700.0f;
        switch (o->var1) {
        case 0:
        case 1:
        case 5:
            v[1] = 0.0f;
            o->var3 = (Rand_Next(1) & 0x7FF) - 0x400;
            o->fvar = 1.0f;
            break;
        case 2:
            v[1] = 7300.0f + (Rand_Next(1) & 0x3FF);
            o->var3 = (Rand_Next(1) & 0xFFF) - 0x800;
            o->fvar = 1.0f;
            break;
        case 3:
            v[1] = 6000.0f + (Rand_Next(1) & 0x3FF) * 3;
            o->var3 = (Rand_Next(1) & 0xFFF) - 0x800;
            o->fvar = 1.0f;
            break;
        case 4:
            v[1] = 7000.0f + (Rand_Next(1) & 0x7FF);
            o->var3 = 0;
            o->fvar = 0.7f + 0.0003f * (Rand_Next(1) & 0x3FF);
            break;
        }
        Vec_RotateY(v, (6.2831855f * Rand_Next(1)) / 65536.0f);
        o->pos[0] = 10000.0f + v[0];
        o->pos[1] = y + v[1];
        o->pos[2] = 10000.0f + v[2];
        if (o->var1 == 5) {
            StageObj_K11_CreateAtScaled(o->pos, 0, 4, 0.15f);
            StageObj_K11_CreateAtScaled(o->pos, 0, 9, 1.0f);
            StageObj_K11_CreateAtScaled(o->pos, 0, 8, 1.0f);
            StageObj_K11_CreateAtScaled(o->pos, 0, 0xB, 1.0f);
        }
        break;
    case 1:
        if (--o->timer <= 0) {
            o->var0 = 0;
            o->timer = (Rand_Next(1) & 0x3F) + 1;
            break;
        }
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        if (o->var1 == 5) {
            OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
        } else {
            OT_AddByDepth(gDrawOt8, o->ds, 8, 0);
        }
        break;
    }
}

void AreaObj_K16_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void AreaObj_K16_Free(Obj *o)
{
    Obj_Free(o);
}

/* Draws the object's stage model (pair b[6] of the stage's model table, from the fourth entry on;
 * the fifth once +8 reaches 6), scaled, turned about Z and faded in over the first 4 frames. */
void AreaObj_K16_Draw(DrawSlot *ds)
{
    u8 *b = ds->owner;
    Model *stage = gAreaSubModels[0];
    f32 m[16];
    ModelMatSet *mdl;
    s32 a;
    s32 off;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x60, 0);
        Mtx_Scale(m, *(f32 *) (b + 0x30), *(f32 *) (b + 0x30), *(f32 *) (b + 0x30));
        Mtx_RotateZ(m, (6.2831855f * *(s16 *) (b + 0xA)) / 65536.0f);
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Mtx_Concat3x3(m, gCamMatrix);
        if (*(s16 *) (b + 8) < 4) {
            a = (u32) (255.0f * (*(s16 *) (b + 8) / 4.0f));
        } else {
            a = 0xFF;
        }
        Gs_SetState(0x67, ((u8) a << 24) | 0xFFFFFF);
        Gs_SetState(0x1A, (s32) m);
        /* a byte offset of its own: the game adds it before the table's address */
        mdl = stage->matSets;
        off = b[6] * 0x118;
        mdl = (ModelMatSet *) (off + (s32) mdl + 0x1A4);
        if (*(s16 *) (b + 8) < 6) {
            mdl++;
        }
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* unk_6F8A10's .data (0x8B0A10-0x8B14A8) */
extern u32 D_008F5290[];
extern u32 D_008F52C0[];
extern u32 D_008F52D8[];
extern u32 D_008F52F0[];
extern u32 D_008F5320[];
extern u32 D_008F5330[];
extern u32 D_008F5360[];
extern u32 D_008F5370[];
extern u32 D_008F5390[];
extern u32 D_008F53B8[];
extern u32 D_008F53C8[];
extern u32 D_008F53D8[];
extern u32 D_008F53F0[];
extern u32 D_008F5420[];
extern u32 D_008F5428[];
extern u32 D_008F5440[];
extern u32 D_008F5470[];
extern u32 D_008F54A0[];
extern u32 D_008F54B0[];
extern u32 D_008F54E0[];
extern u32 D_008F54F0[];
extern u32 D_008F5510[];
extern u32 D_008F5540[];
extern u32 D_008F5548[];
extern u32 D_008F5560[];
extern u32 D_008F5590[];
extern u32 D_008F55A0[];
extern u32 D_008F55D0[];
extern u32 D_008F55E0[];
extern u32 D_008F5610[];
extern u32 D_008F5620[];
extern u32 D_008F5640[];
extern u32 D_008F5660[];
extern u32 D_008F5680[];
extern u32 D_008F56A0[];
extern u32 D_008F56D0[];
extern u32 D_008F5700[];
extern u32 D_008F5720[];
extern u32 D_008F5748[];
extern u32 D_008F5750[];
extern u32 D_008F5768[];
extern u32 D_008F5780[];
extern u32 D_008F57B0[];
extern u32 D_008F57C0[];
extern u32 D_008F57D8[];
extern u32 D_008F57F0[];
extern u32 D_008F5818[];
extern u32 D_008F5830[];
extern u32 D_008F5848[];
extern u32 D_008F5860[];
extern u32 D_008F5888[];
extern u32 D_008F58A0[];
f32 AreaObj_K09_Type8Pos[32] = {
#include "game/008B0A10.inc"
};
f32 AreaObj_K09_Type11Pos[6] = {
#include "game/008B0A90.inc"
};
f32 gAreaObjK0A_PosX = {
#include "game/008B0AA8.inc"
};
f32 gAreaObjK0A_PosZ = {
#include "game/008B0AB0.inc"
};
f32 AreaObj_K0A_Offsets[12] = {
#include "game/008B0AC0.inc"
};
f32 AreaObj_K0B_Pos[5][3] = {
#include "game/008B0AF0.inc"
};
s16 AreaObj_K0B_ModelsA[8] = {
#include "game/008B0B30.inc"
};
u16 AreaObj_K0B_Angles[8] = {
#include "game/008B0B40.inc"
};
f32 AreaObj_K0B_Rise[6] = {
#include "game/008B0B50.inc"
};
s16 AreaObj_K0B_Spin[6] = {
#include "game/008B0B68.inc"
};
f32 AreaObj_K0B_SePos[8] = {
#include "game/008B0B80.inc"
};
f32 AreaObj_K0B_EffectPos[3][3] = {
#include "game/008B0BA0.inc"
};
s16 AreaObj_K0B_ModelsB[6] = {
#include "game/008B0BC8.inc"
};
f32 AreaObj_K0E_UV[16][2] = {
#include "game/008B0BE0.inc"
};
f32 AreaObj_K0E_Pos[4][3] = {
#include "game/008B0C60.inc"
};
f32 AreaObj_K0E_Lights1[5][3] = {
#include "game/008B0C90.inc"
};
f32 AreaObj_K0E_Lights2[5][3] = {
#include "game/008B0CD0.inc"
};
f32 AreaObj_K0E_Lights3[4][3] = {
#include "game/008B0D10.inc"
};
f32 AreaObj_K0E_Lights4[4][3] = {
#include "game/008B0D40.inc"
};
f32 AreaObj_K0E_Lights5[4][3] = {
#include "game/008B0D70.inc"
};
s16 AreaObj_K0E_On0[4] = {
#include "game/008B0DA0.inc"
};
s16 AreaObj_K0E_On1[4] = {
#include "game/008B0DA8.inc"
};
s16 AreaObj_K0E_On2[4] = {
#include "game/008B0DB0.inc"
};
s16 AreaObj_K0E_On3[4] = {
#include "game/008B0DB8.inc"
};
s16 AreaObj_K0E_On4[4] = {
#include "game/008B0DC0.inc"
};
s16 AreaObj_K0E_On5[4] = {
#include "game/008B0DC8.inc"
};
s16 AreaObj_K0E_On6[8] = {
#include "game/008B0DD0.inc"
};
f32 AreaObj_K10_Pos[5][3] = {
#include "game/008B0DE0.inc"
};
f32 AreaObj_K10_Scales[6] = {
#include "game/008B0E20.inc"
};
s16 AreaObj_K10_Models[6] = {
#include "game/008B0E38.inc"
};
u8 AreaObj_K11_TileIdsA[32] = {
#include "game/008B0E50.inc"
};
u8 AreaObj_K11_TileIdsB[80] = {
#include "game/008B0E70.inc"
};
u16 AreaObj_K11_Tiles[72] = {
#include "game/008B0EC0.inc"
};
u32 AreaObj_K13_Models_1[2] = {
#include "game/008B0F50.inc"
};
f32 AreaObj_K13_Params_2[3] = {
#include "game/008B0F58.inc"
};
u32 AreaObj_K13_Models_2[2] = {
#include "game/008B0F68.inc"
};
f32 AreaObj_K13_Params_3[4] = {
#include "game/008B0F70.inc"
};
u32 AreaObj_K13_Models_3[4] = {
#include "game/008B0F80.inc"
};
f32 AreaObj_K13_Params_4[10] = {
#include "game/008B0F90.inc"
};
u32 AreaObj_K13_Models_4[2] = {
#include "game/008B0FB8.inc"
};
f32 AreaObj_K13_Params_5[4] = {
#include "game/008B0FC0.inc"
};
u32 AreaObj_K13_Models_5[2] = {
#include "game/008B0FD0.inc"
};
f32 AreaObj_K13_Params_6[3] = {
#include "game/008B0FD8.inc"
};
u32 AreaObj_K13_Models_6[2] = {
#include "game/008B0FE8.inc"
};
f32 AreaObj_K13_Params_7[6] = {
#include "game/008B0FF0.inc"
};
u32 AreaObj_K13_Models_7[2] = {
#include "game/008B1008.inc"
};
u32 AreaObj_K13_Models_9[2] = {
#include "game/008B1010.inc"
};
u32 AreaObj_K13_Models_10[2] = {
#include "game/008B1018.inc"
};
u32 AreaObj_K13_Models_13[2] = {
#include "game/008B1020.inc"
};
u32 AreaObj_K13_Models_16[2] = {
#include "game/008B1028.inc"
};
u32 AreaObj_K13_Models_17[2] = {
#include "game/008B1030.inc"
};
u32 AreaObj_K13_Models_18[2] = {
#include "game/008B1038.inc"
};
u32 AreaObj_K13_Models_19[2] = {
#include "game/008B1040.inc"
};
u32 AreaObj_K13_Models_21[2] = {
#include "game/008B1048.inc"
};
u32 AreaObj_K13_Models_0[2] = {
#include "game/008B1050.inc"
};
f32 AreaObj_K13_Params_25[3] = {
#include "game/008B1058.inc"
};
u32 AreaObj_K13_Models_25[2] = {
#include "game/008B1068.inc"
};
u32 AreaObj_K13_Models_26[2] = {
#include "game/008B1070.inc"
};
u32 AreaObj_K13_Models_28[2] = {
#include "game/008B1078.inc"
};
u32 AreaObj_K13_Models_32[2] = {
#include "game/008B1080.inc"
};
u32 AreaObj_K13_Models_33[2] = {
#include "game/008B1088.inc"
};
u32 AreaObj_K13_Models_34[2] = {
#include "game/008B1090.inc"
};
u32 AreaObj_K13_Models_35[2] = {
#include "game/008B1098.inc"
};
u32 AreaObj_K13_Models_36[4] = {
#include "game/008B10A0.inc"
};
f32 AreaObj_K13_Params_37[10] = {
#include "game/008B10B0.inc"
};
u32 AreaObj_K13_Models_37[2] = {
#include "game/008B10D8.inc"
};
u32 AreaObj_K13_Models_38[2] = {
#include "game/008B10E0.inc"
};
u32 AreaObj_K13_Models_39[2] = {
#include "game/008B10E8.inc"
};
u32 AreaObj_K13_Models_8[4] = {
#include "game/008B10F0.inc"
};
f32 AreaObj_K13_Params_41[12] = {
#include "game/008B1100.inc"
};
u32 AreaObj_K13_Models_42[2] = {
#include "game/008B1130.inc"
};
u32 AreaObj_K13_Models_43[2] = {
#include "game/008B1138.inc"
};
f32 AreaObj_K13_Params_45[4] = {
#include "game/008B1140.inc"
};
u32 AreaObj_K13_Models_45[2] = {
#include "game/008B1150.inc"
};
u32 AreaObj_K13_Models_46[2] = {
#include "game/008B1158.inc"
};
u32 AreaObj_K13_Models_47[2] = {
#include "game/008B1160.inc"
};
u32 AreaObj_K13_Models_48[2] = {
#include "game/008B1168.inc"
};
u32 AreaObj_K13_Models_49[2] = {
#include "game/008B1170.inc"
};
u32 AreaObj_K13_Models_51[2] = {
#include "game/008B1178.inc"
};
u32 AreaObj_K13_Models_50[2] = {
#include "game/008B1180.inc"
};
u32 AreaObj_K13_Models_52[2] = {
#include "game/008B1188.inc"
};
u32 AreaObj_K13_Models_53[2] = {
#include "game/008B1190.inc"
};
u32 AreaObj_K13_Models_54[2] = {
#include "game/008B1198.inc"
};
f32 *AreaObj_K13_Params[56] = {
    (f32 *)D_008F5440, (f32 *)D_008F5290, AreaObj_K13_Params_2, AreaObj_K13_Params_3,
    AreaObj_K13_Params_4, AreaObj_K13_Params_5, AreaObj_K13_Params_6, AreaObj_K13_Params_7,
    (f32 *)D_008F5720, (f32 *)D_008F52C0, (f32 *)D_008F52D8, (f32 *)D_008F52F0,
    (f32 *)D_008F5330, (f32 *)D_008F5370, AreaObj_K13_Params_2, AreaObj_K13_Params_6,
    (f32 *)D_008F5390, (f32 *)D_008F53B8, (f32 *)D_008F53C8, (f32 *)D_008F53D8,
    (f32 *)D_008F53F0, (f32 *)D_008F5428, (f32 *)D_008F5440, (f32 *)D_008F5470,
    (f32 *)D_008F54B0, AreaObj_K13_Params_25, (f32 *)D_008F54F0, (f32 *)D_008F5510,
    (f32 *)D_008F5548, (f32 *)D_008F5560, (f32 *)D_008F55A0, (f32 *)D_008F55E0,
    (f32 *)D_008F5620, (f32 *)D_008F5640, (f32 *)D_008F5660, (f32 *)D_008F5680,
    (f32 *)D_008F56A0, AreaObj_K13_Params_37, (f32 *)D_008F56D0, (f32 *)D_008F5700,
    (f32 *)D_008F5720, AreaObj_K13_Params_41, (f32 *)D_008F5750, (f32 *)D_008F5768,
    (f32 *)D_008F5780, AreaObj_K13_Params_45, (f32 *)D_008F57C0, (f32 *)D_008F57D8,
    (f32 *)D_008F57F0, (f32 *)D_008F5818, (f32 *)D_008F5848, (f32 *)D_008F5830,
    (f32 *)D_008F5860, (f32 *)D_008F5888, (f32 *)D_008F58A0, (f32 *)D_008F58A0,
};
s16 *AreaObj_K13_Models[56] = {
    (s16 *)AreaObj_K13_Models_0, (s16 *)AreaObj_K13_Models_1, (s16 *)AreaObj_K13_Models_2, (s16 *)AreaObj_K13_Models_3,
    (s16 *)AreaObj_K13_Models_4, (s16 *)AreaObj_K13_Models_5, (s16 *)AreaObj_K13_Models_6, (s16 *)AreaObj_K13_Models_7,
    (s16 *)AreaObj_K13_Models_8, (s16 *)AreaObj_K13_Models_9, (s16 *)AreaObj_K13_Models_10, (s16 *)D_008F5320,
    (s16 *)D_008F5360, (s16 *)AreaObj_K13_Models_13, (s16 *)AreaObj_K13_Models_2, (s16 *)AreaObj_K13_Models_6,
    (s16 *)AreaObj_K13_Models_16, (s16 *)AreaObj_K13_Models_17, (s16 *)AreaObj_K13_Models_18, (s16 *)AreaObj_K13_Models_19,
    (s16 *)D_008F5420, (s16 *)AreaObj_K13_Models_21, (s16 *)AreaObj_K13_Models_0, (s16 *)D_008F54A0,
    (s16 *)D_008F54E0, (s16 *)AreaObj_K13_Models_25, (s16 *)AreaObj_K13_Models_26, (s16 *)D_008F5540,
    (s16 *)AreaObj_K13_Models_28, (s16 *)D_008F5590, (s16 *)D_008F55D0, (s16 *)D_008F5610,
    (s16 *)AreaObj_K13_Models_32, (s16 *)AreaObj_K13_Models_33, (s16 *)AreaObj_K13_Models_34, (s16 *)AreaObj_K13_Models_35,
    (s16 *)AreaObj_K13_Models_36, (s16 *)AreaObj_K13_Models_37, (s16 *)AreaObj_K13_Models_38, (s16 *)AreaObj_K13_Models_39,
    (s16 *)AreaObj_K13_Models_8, (s16 *)D_008F5748, (s16 *)AreaObj_K13_Models_42, (s16 *)AreaObj_K13_Models_43,
    (s16 *)D_008F57B0, (s16 *)AreaObj_K13_Models_45, (s16 *)AreaObj_K13_Models_46, (s16 *)AreaObj_K13_Models_47,
    (s16 *)AreaObj_K13_Models_48, (s16 *)AreaObj_K13_Models_49, (s16 *)AreaObj_K13_Models_50, (s16 *)AreaObj_K13_Models_51,
    (s16 *)AreaObj_K13_Models_52, (s16 *)AreaObj_K13_Models_53, (s16 *)AreaObj_K13_Models_54, (s16 *)AreaObj_K13_Models_54,
};
s16 AreaObj_K13_Pos[56] = {
#include "game/008B1360.inc"
};
f32 AreaObj_K13_Scroll[16][2] = {
#include "game/008B13D0.inc"
};
f32 AreaObj_K14_PosA[2][3] = {
#include "game/008B1450.inc"
};
f32 AreaObj_K14_PosB[2][3] = {
#include "game/008B1470.inc"
};
s16 AreaObj_K14_Models[4] = {
#include "game/008B1488.inc"
};
f32 AreaObj_K14_SePos[6] = {
#include "game/008B1490.inc"
};
