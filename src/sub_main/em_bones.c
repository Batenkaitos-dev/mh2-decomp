/*
 * The monsters' skeletons and riding, and the spark effect.
 *
 * - Bones: Em_UpdateMatrix builds a monster's matrix (scale, angles, position) and its bones'
 *   (Bone_UpdateTree, after Em_AdjustKindBones' per-kind poses: the Rathalos family's neck bend,
 *   the Kut-Ku's ears, the Plesioth's fins ...); Em_CopyBoneMatrices keeps a copy of the matrices
 *   (gEmBoneMtxBuf) that Em_PosInFrame uses.
 * - Riding: the large monsters a hunter can stand on (the Lao-Shan Lung's, the Shen Gaoren's and the
 *   Yama Tsukami's kinds) list quads on their bones (gEmRideQuads). Em_CarryRiders carries the hunters
 *   standing on one and lands the others that fall onto it; Em_HitQuadsVertical finds the quad under a
 *   point. Tri_SegmentHit and Tri_ContainsPointAngles test the quads' triangles.
 * - Em_RunAll runs every active monster's update and gives it a draw slot.
 * - The spark effect (effect kind 8, SparkEmitter): SparkFx_Update's states start, run, end and free
 *   its sparks, which follow key tracks (gSparkFxKeys) and draw a model of the common set
 *   (Spark_Draw).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "prim.h"
#include "sprite.h"
#include "fxobj.h"
#include "model.h"
#include "render_state.h"

/* Built at optimization level 4: Tri_SegmentHit compiles as the game's code only at that level. */
#pragma optimization_level 4

/* prototypes */
u16 Rand_Next(s32 a);
void Text_SetSize(s32 size);
struct StageObj *StageObj_AllocTail(s32 n);
void StageObj_Free(struct StageObj *p);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void DrawSlot_Free(s32 id);
void * Stage_FindData(u16 id);
struct SparkEmitter;
void SparkFx_Start(struct SparkEmitter *e);
void SparkFx_Run(struct SparkEmitter *e);
void SparkFx_End(struct SparkEmitter *e);
void SparkFx_Free(SparkEmitter *e);
void Fish_Start(FxObj *o);
struct FxObj;
void Fish_Update(struct FxObj *o);
void Fish_End(FxObj *o);
void Fish_Free(FxObj *o);
void Lure_Start(struct FxObj *o);
void Lure_Update(struct FxObj *o);
void Lure_End(FxObj *o);
void Lure_Free(FxObj *o);
void CountdownFx_Start(FxObj *o);
void CountdownFx_Run(FxObj *o);
void CountdownFx_End(FxObj *o);
void CountdownFx_Free(FxObj *o);
void Countdown_Draw(s16 t);
void Letterbox_Draw(s8);
s32 Gs_SetState(u8 reg, u32 v);
s16 Effect_MapTexId(s16 id);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 f);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Vec_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Inverse(f32 *dst, f32 *src);
s32 Tri_SegmentHit(f32 *tri, f32 *p0, f32 *p1, f32 *out);
extern Em gEms[];
struct SparkObj;
void Lure_Draw(struct DrawSlot *slot);
void Vec_RotateY(f32 *v, f32 angle);
void Vib_StartLocal(void *port, s32 i);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
/* end of prototypes */


s32 Prim_FillTri(PrimTri *t);


s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);

void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Math_Acos(f32 x);
/* the calls cast to u16: this unit's code takes the value as u16 */
u32 Math_RadToAngle(f32 r);

/* 1 when p is inside triangle tri (three points): the angles the sides subtend at p add up to
 * a full turn (at least 0xF000). */
s32 Tri_ContainsPointAngles(f32 *tri, f32 *point)
{
    f32 toA[3];
    f32 toB[3];
    f32 toC[3];
    f32 angleAB;
    f32 angleBC;
    f32 angleCA;

    toA[0] = tri[0] - point[0];
    toA[1] = tri[1] - point[1];
    toA[2] = tri[2] - point[2];
    toB[0] = tri[3] - point[0];
    toB[1] = tri[4] - point[1];
    toB[2] = tri[5] - point[2];
    toC[0] = tri[6] - point[0];
    toC[1] = tri[7] - point[1];
    toC[2] = tri[8] - point[2];
    Vec_Normalize(toA);
    Vec_Normalize(toB);
    Vec_Normalize(toC);
    angleAB = Vec_Dot(toA, toB);
    angleBC = Vec_Dot(toB, toC);
    angleCA = Vec_Dot(toC, toA);
    if (angleAB > 1.0f) {
        angleAB = 1.0f;
    }
    if (angleBC > 1.0f) {
        angleBC = 1.0f;
    }
    if (angleCA > 1.0f) {
        angleCA = 1.0f;
    }
    if (angleAB < -1.0f) {
        angleAB = -1.0f;
    }
    if (angleBC < -1.0f) {
        angleBC = -1.0f;
    }
    if (angleCA < -1.0f) {
        angleCA = -1.0f;
    }
    angleAB = Math_Acos(angleAB);
    angleBC = Math_Acos(angleBC);
    angleCA = Math_Acos(angleCA);
    if ((u16)Math_RadToAngle(angleAB) + (u16)Math_RadToAngle(angleBC) + (u16)Math_RadToAngle(angleCA) >= 0xF000) {
        return 1;
    }
    return 0;
}

void Vec_Cross(f32 *out, f32 *a, f32 *b);

/* Where segment start-end crosses the plane of triangle tri (three points): out, and returns 1 when
 * that point is inside the triangle and between start and end; returns 2 when start lowered by 30
 * lies on the plane inside the triangle (out is that point); else 0. */
s32 Tri_SegmentHit(f32 *tri, f32 *start, f32 *end, f32 *out)
{
    f32 toStart[3];
    f32 toEnd[3];
    f32 normal[3];
    f32 edgeA[3];
    f32 edgeB[3];
    s32 result = 0;
    f32 *normalY;
    f32 *normalZ;
    f32 planeDist;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 along;

    edgeA[0] = tri[0] - tri[3];
    edgeA[1] = tri[1] - tri[4];
    edgeA[2] = tri[2] - tri[5];
    edgeB[0] = tri[6] - tri[3];
    edgeB[1] = tri[7] - tri[4];
    edgeB[2] = tri[8] - tri[5];
    Vec_Cross(normal, edgeB, edgeA);
    Vec_Normalize(normal);
    normalY = &normal[1];
    normalZ = &normal[2];
    planeDist = -(normal[0] * tri[0]) - *normalY * tri[1] - *normalZ * tri[2];
    dx = end[0] - start[0];
    dy = end[1] - start[1];
    dz = end[2] - start[2];
    if (planeDist + (normal[0] * start[0] + *normalY * (start[1] - 30.0f) + *normalZ * start[2]) == 0.0f) {
        out[0] = start[0];
        out[1] = start[1] - 30.0f;
        out[2] = start[2];
        if (Tri_ContainsPointAngles(tri, out)) {
            return 2;
        }
    }
    along = -(planeDist + (normal[0] * start[0] + *normalY * start[1] + *normalZ * start[2]));
    along /= normal[0] * dx + *normalY * dy + *normalZ * dz;
    out[0] = start[0] + dx * along;
    out[1] = start[1] + dy * along;
    out[2] = start[2] + dz * along;
    toStart[0] = start[0] - out[0];
    toStart[1] = start[1] - out[1];
    toStart[2] = start[2] - out[2];
    toEnd[0] = end[0] - out[0];
    toEnd[1] = end[1] - out[1];
    toEnd[2] = end[2] - out[2];
    Vec_Normalize(toStart);
    Vec_Normalize(toEnd);
    if (Vec_Dot(toEnd, toStart) < 0.0f && Tri_ContainsPointAngles(tri, out)) {
        result = 1;
    }
    return result;
}

extern f32 gEmBoneMtxBuf[][16];
extern f32 gEmBonePosBuf[][3];
void Mtx_Copy(f32 *dst, f32 *src);

/* Copies each bone matrix of an active monster to gEmBoneMtxBuf[i] (Mtx_Copy) and its
 * translation to gEmBonePosBuf[i]. */
void Em_CopyBoneMatrices(Em *em)
{
    s16 i;
    EmBone *bone;
    s16 n;

    if (em->active == 0 || em->visible == 0) {
        return;
    }
    bone = em->model->bones;
    n = bone->count;
    for (i = 0; i < n; i++, bone++) {
        gEmBonePosBuf[i][0] = bone->world[12];
        gEmBonePosBuf[i][1] = bone->world[13];
        gEmBonePosBuf[i][2] = bone->world[14];
        Mtx_Copy(gEmBoneMtxBuf[i], bone->world);
    }
}

void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_ClearRotation(f32 *m);
void Mtx_Transpose3x3(f32 *m);
void Mtx_CopyRot(f32 *m, f32 *src);
f32 Math_Angle16ToRad(u16 a);
f32 *Em_BoneMatrixB(void *obj, s16 bone);
void EmYamaTsukami_ScaleBones(Em *em, EmBone *bones);
f32 EmHermitaur_RageFxRatio(Em *em);
extern EmBendRow *gEmBendOrigins[];

/* The per-kind poses set on the bones before the skeleton is updated (Em_UpdateMatrix):
 * - the Lao-Shan Lungs, the Rathians and the Rathaloses: the four bend bones (gEmBendOrigins' row)
 *   turn by bendAngle;
 * - the Kut-Kus and the Garuga: the ears (bones 0x25-0x27) open over 30 frames while the monster is
 *   in combat with at least a fifth of its health, and close otherwise (EmFlyerWork.earOpen);
 * - the Khezus: bone 0x25 takes bone 0x23's rotation, transposed;
 * - the Kirin: bones 0x18 and 0x19 spin about X (a turn every 16 frames, the second 5 frames behind),
 *   and stop in state group 5;
 * - the Plesioths and the Cephadrome family: the fin bones fold and wave (EmPlesiothWork.finRot);
 * - the Shen Gaoren: bones 11-14 stretch by its work's ease values;
 * - kind 0x38: bone 2 spins on all three axes;
 * - the Yama Tsukamis: EmYamaTsukami_ScaleBones;
 * - the Shogun Ceanataur: bones 6 and 10 scale and turn by EmHermitaur_RageFxRatio, one way calm and
 *   the other enraged;
 * - the Rajang: bone 15 swells with its rage timer. */
void Em_AdjustKindBones(Em *em, EmBone *boneList)
{
    EmBone *bones = boneList;
    f32 mtx[16];
    f32 transposed[16];
    EmBendRow *bend;
    EmFlyerWork *flyer;
    EmPlesiothWork *finWork;
    f32 *boneMtx;
    f32 phase;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    bend = gEmBendOrigins[em->type];
    switch (em->type) {
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        Mtx_RotateY(bones[bend->bendBone[3]].local, Math_Angle16ToRad((u16)em->bendAngle[3]));
        Mtx_RotateY(bones[bend->bendBone[2]].local, Math_Angle16ToRad((u16)em->bendAngle[2]));
        Mtx_RotateY(bones[bend->bendBone[1]].local, Math_Angle16ToRad((u16)em->bendAngle[1]));
        Mtx_RotateY(bones[bend->bendBone[0]].local, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_YIAN_GARUGA:
        flyer = (EmFlyerWork *)&em->work;
        if (em->combat != 1 || em->hp < (s16)(0.2f * em->hpMax)) {
            if (flyer->earOpen > 0) {
                flyer->earOpen--;
            }
        } else {
            if (flyer->earOpen < 30) {
                flyer->earOpen++;
            }
        }
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x25));
        Mtx_ClearRotation(mtx);
        Mtx_RotateX(mtx, -1.308997f + 0.043633234f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x25), mtx);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x26));
        Mtx_ClearRotation(mtx);
        Mtx_RotateY(mtx, 0.5235988f - 0.017453292f * (u32)flyer->earOpen);
        Mtx_RotateZ(mtx, 0.87266463f - 0.072722055f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x26), mtx);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x27));
        Mtx_ClearRotation(mtx);
        Mtx_RotateY(mtx, -0.5235988f + 0.017453292f * (u32)flyer->earOpen);
        Mtx_RotateZ(mtx, -0.87266463f + 0.072722055f * (u32)flyer->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x27), mtx);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        Mtx_Copy(transposed, Em_BoneMatrixB(em, 0x23));
        Mtx_Transpose3x3(transposed);
        Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x25));
        Mtx_CopyRot(mtx, transposed);
        Mtx_Copy(Em_BoneMatrixB(em, 0x25), mtx);
        break;
    case EM_KIND_KIRIN:
        if (em->stateGroup != 5) {
            Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x18));
            Mtx_ClearRotation(mtx);
            Mtx_RotateX(mtx, (6.2831855f * (f32)-((gScene.frame & 0xF) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 0x18), mtx);
            Mtx_Copy(mtx, Em_BoneMatrixB(em, 0x19));
            Mtx_ClearRotation(mtx);
            Mtx_RotateX(mtx, (6.2831855f * (f32)-(((gScene.frame & 0xF) - 5) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 0x19), mtx);
        } else {
            Mtx_ClearRotation(Em_BoneMatrixB(em, 0x18));
            Mtx_ClearRotation(Em_BoneMatrixB(em, 0x19));
        }
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        bones[37].pose[0] = 1.0f;
        bones[37].pose[1] = 1.0f;
        bones[37].pose[2] = 1.0f;
        bones[38].pose[0] = 1.0f;
        bones[38].pose[1] = 1.0f;
        bones[38].pose[2] = 1.0f;
        bones[39].pose[0] = 1.0f;
        bones[39].pose[1] = 1.0f;
        bones[39].pose[2] = 1.0f;
        bones[16].pose[0] = 1.0f;
        bones[16].pose[1] = 1.0f;
        bones[16].pose[2] = 1.0f;
        finWork = (EmPlesiothWork *)&em->work;
        Mtx_RotateX(bones[37].local, Math_Angle16ToRad(finWork->finRot[0].fold));
        Mtx_RotateX(bones[38].local, Math_Angle16ToRad(finWork->finRot[1].fold));
        Mtx_RotateX(bones[39].local, Math_Angle16ToRad(finWork->finRot[2].fold));
        Mtx_RotateX(bones[16].local, Math_Angle16ToRad(finWork->finRot[3].fold));
        Mtx_RotateY(bones[37].local, Math_Angle16ToRad(finWork->finRot[0].wave));
        Mtx_RotateY(bones[38].local, Math_Angle16ToRad(finWork->finRot[1].wave));
        Mtx_RotateY(bones[39].local, Math_Angle16ToRad(finWork->finRot[2].wave));
        Mtx_RotateY(bones[16].local, Math_Angle16ToRad(finWork->finRot[3].wave));
        break;
    case EM_KIND_SHEN_GAOREN:
        bones[11].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[0];
        bones[12].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[1];
        bones[13].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[2];
        bones[14].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[3];
        break;
    case 0x38:
        boneMtx = Em_BoneMatrixB(em, 2);
        Mtx_ClearRotation(boneMtx);
        phase = (6.2831855f * (f32)(gScene.frame << 13)) / 65536.0f;
        Mtx_RotateXYZ(boneMtx, phase, phase, phase);
        break;
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_ScaleBones(em, bones);
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        phase = EmHermitaur_RageFxRatio(em);
        if (em->rage == 0) {
            scaleX = 0.9f + 0.1f * phase;
            scaleY = 0.85f + 0.15f * phase;
            scaleZ = 0.75f + 0.25f * phase;
            bones[6].pose[0] = scaleX;
            bones[6].pose[1] = scaleY;
            bones[6].pose[2] = scaleZ;
            bones[10].pose[0] = scaleX;
            bones[10].pose[1] = scaleY;
            bones[10].pose[2] = scaleZ;
            Mtx_ClearRotation(bones[6].local);
            Mtx_ClearRotation(bones[10].local);
            phase = (6.2831855f * (170.0f - 170.0f * phase)) / 360.0f;
            Mtx_RotateX(bones[6].local, phase);
            Mtx_RotateX(bones[10].local, phase);
            Mtx_RotateY(bones[6].local, 0.0f);
            Mtx_RotateY(bones[10].local, 0.0f);
            Mtx_RotateZ(bones[6].local, 0.0f);
            Mtx_RotateZ(bones[10].local, 0.0f);
        } else {
            scaleX = 1.0f - 0.1f * phase;
            scaleY = 1.0f - 0.15f * phase;
            scaleZ = 1.0f - 0.25f * phase;
            bones[6].pose[0] = scaleX;
            bones[6].pose[1] = scaleY;
            bones[6].pose[2] = scaleZ;
            bones[10].pose[0] = scaleX;
            bones[10].pose[1] = scaleY;
            bones[10].pose[2] = scaleZ;
            Mtx_ClearRotation(bones[6].local);
            Mtx_ClearRotation(bones[10].local);
            phase = (6.2831855f * (170.0f * phase)) / 360.0f;
            Mtx_RotateX(bones[6].local, phase);
            Mtx_RotateX(bones[10].local, phase);
            Mtx_RotateY(bones[6].local, 0.0f);
            Mtx_RotateY(bones[10].local, 0.0f);
            Mtx_RotateZ(bones[6].local, 0.0f);
            Mtx_RotateZ(bones[10].local, 0.0f);
        }
        break;
    case EM_KIND_RAJANG:
        phase = (f32)((EmBlangongaWork *)&em->work)->rageTimer / 15.0f;
        bones[15].pose[0] = 1.0f + 6.0f * phase;
        bones[15].pose[1] = 1.0f + 10.0f * phase;
        bones[15].pose[2] = 1.0f;
        break;
    }
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Identity(f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Concat(f32 *m, f32 *a);
void Mtx_RotateY(f32 *m, f32 a);
f32 Math_Angle16ToRad(u16 a);

/* The world matrices of bone and its subtree: the bone's pose scale times the parent's scale (which the
 * children inherit), its local matrix, then the parent's matrix; bones 0x21-0x24 then turn by
 * bendAngle[3] down to bendAngle[0]. */
void Bone_UpdateTree(Em *em, EmBone *bone, f32 *parent, f32 parentScaleX, f32 parentScaleY, f32 parentScaleZ)
{
    f32 unscale[16];
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    scaleX = bone->pose[0] * parentScaleX;
    scaleY = bone->pose[1] * parentScaleY;
    scaleZ = bone->pose[2] * parentScaleZ;
    Mtx_Scale(bone->world, scaleX, scaleY, scaleZ);
    Mtx_Identity(unscale);
    unscale[0] = 1.0f / parentScaleX;
    unscale[5] = 1.0f / parentScaleY;
    unscale[10] = 1.0f / parentScaleZ;
    unscale[12] = bone->local[12];
    unscale[13] = bone->local[13];
    unscale[14] = bone->local[14];
    Mtx_Concat3x3(bone->world, bone->local);
    Mtx_Concat(bone->world, unscale);
    Mtx_Concat(bone->world, parent);
    switch (bone->index) {
    case 0x21:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[3]));
        break;
    case 0x22:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[2]));
        break;
    case 0x23:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[1]));
        break;
    case 0x24:
        Mtx_RotateY(bone->world, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    }
    if (bone->child != NULL) {
        Bone_UpdateTree(em, bone->child, bone->world, scaleX, scaleY, scaleZ);
    }
    if (bone->sibling != NULL) {
        Bone_UpdateTree(em, bone->sibling, parent, parentScaleX, parentScaleY, parentScaleZ);
    }
}

void Bone_UpdateTree(Em *em, EmBone *b, f32 *parent, f32 x, f32 y, f32 z);

/* Bone_UpdateTree from the root bones under matrix parent, with the scale 1. */
void Bone_UpdateTreeUnscaled(void *em, void *bones, f32 *parent)
{
    Bone_UpdateTree(em, bones, parent, 1.0f, 1.0f, 1.0f);
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Em_AdjustKindBones(Em *em, EmBone *bones);
void Bone_UpdateWorldScaledRoot(EmBone *bones, f32 *m);

/* Builds the monster's matrix from its scale, angles and position into worldMtx[0], then its bones'
 * (Em_AdjustKindBones, then Bone_UpdateWorldScaledRoot; the Chameleos: Bone_UpdateTreeUnscaled). */
void Em_UpdateMatrix(Em *em)
{
    Model *model = em->model;
    f32 mtx[16];
    f32 scaleMtx[16];

    Mtx_Scale(scaleMtx, em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx);
    Mtx_SetTrans(mtx, em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx, scaleMtx);
    Mtx_Copy(em->worldMtx[0], mtx);
    Em_AdjustKindBones(em, model->bones);
    if (em->type == EM_KIND_CHAMELEOS) {
        Bone_UpdateTreeUnscaled(em, model->bones, mtx);
        return;
    }
    Bone_UpdateWorldScaledRoot(model->bones, mtx);
}

extern f32 gEmBoneMtxBuf[][16];
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* out = obj's position (a monster's, or a player's: the same offset) in the frame of matrix i of
 * gEmBoneMtxBuf. em (the monster carrying obj) is unused. */
void Em_PosInFrame(Em *obj, Em *em, s16 i, f32 *out)
{
    f32 delta[3];
    f32 origin[3];
    f32 inv[16];
    f32 *boneMtx = gEmBoneMtxBuf[i];

    Mtx_Inverse(inv, boneMtx);
    Mtx_GetTrans(origin, boneMtx);
    delta[0] = obj->posX - origin[0];
    delta[1] = obj->posY - origin[1];
    delta[2] = obj->posZ - origin[2];
    Vec_Rotate(out, delta, inv);
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
s32 EmYamaTsukami_IsQuakeFreeMotion(Em *em);
s32 Obj_SameArea(void *obj, Player *b);
s32 Obj_InCurrentArea(void *obj);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_ReturnToIdle(Player *p, s32 a, s32 b, s32 c);
void Player_StopMove(void *obj);
extern f32 *gEmRideQuads[];

/* Carries the hunters standing on a monster that can be ridden (the Lao-Shan Lungs, the Shen Gaoren and
 * the Yama Tsukamis): a rider (rideState 1 or 3) on this monster moves with its bone (rideDelta: how
 * far). Then each hunter in the area who isn't reacting, fainted or in an event is dropped onto the
 * monster's quads (gEmRideQuads) from just above: on a hit it stands on the quad's bone (rideState 1,
 * or 3 and a ledge hang when it jumped up onto it); without a hit it stops riding. A Yama Tsukami
 * lands no one while EmYamaTsukami_IsQuakeFreeMotion is 0. */
void Em_CarryRiders(Em *em)
{
    f32 corner[3];
    f32 origin[3];
    f32 oldPos[3];
    f32 rotated[3];
    f32 top[3];
    f32 bottom[3];
    f32 tri[3][3];
    f32 quad[4][3];
    Player *pl;
    EmBone *bones;
    s32 i;
    s32 hits;
    s32 skip;
    s32 k;
    f32 *list;
    s32 j;
    f32 *quadCorner;
    u8 yamaHolds;

    pl = gPlayers;
    yamaHolds = 0;
    bones = em->model->bones;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG || em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
        if ((u8)(s32)Player_InCurrentArea(em)) {
            if ((em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) && EmYamaTsukami_IsQuakeFreeMotion(em) == 0) {
                yamaHolds = 1;
            }
            skip = yamaHolds;
            for (i = 0; i < SCENE_PLAYERS; i++, pl++) {
                if (!(u8)Obj_SameArea(pl, (Player *)em) || !(u8)Obj_InCurrentArea(pl)) {
                    continue;
                }
                if (pl->rideState == 1 || pl->rideState == 3) {
                    if (pl->rideEm == em->slotNo) {
                        Vec_Copy(oldPos, pl->pos);
                        k = pl->rideBone;
                        Em_PosInFrame((Em *)pl, em, k, origin);
                        Vec_Rotate(corner, origin, bones[k].world);
                        Mtx_GetTrans(origin, bones[k].world);
                        pl->pos[0] = corner[0] + origin[0];
                        pl->pos[1] = corner[1] + origin[1];
                        pl->pos[2] = corner[2] + origin[2];
                        pl->rideDelta[0] = pl->pos[0] - oldPos[0];
                        pl->rideDelta[1] = pl->pos[1] - oldPos[1];
                        pl->rideDelta[2] = pl->pos[2] - oldPos[2];
                    }
                }
                if (pl->stateGroup != PL_GROUP_REACT && pl->stateGroup != PL_GROUP_FAINT && pl->stateGroup != PL_GROUP_EVENT) {
                    if (skip) {
                        continue;
                    }
                    if (pl->vel[1] > 0.0f && pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_LEAP_OFF_LEDGE && (s32)pl->motionTime < 0x15 && pl->stateVar0 == 0) {
                        continue;
                    }
                    if (pl->vel[1] > 0.0f) {
                        top[1] = 230.0f + pl->pos[1];
                        bottom[1] = top[1] - 60.0f;
                    } else {
                        top[1] = 30.0f + pl->pos[1];
                        bottom[1] = top[1] - 60.0f;
                    }
                    top[0] = pl->pos[0];
                    hits = 0;
                    top[2] = pl->pos[2];
                    bottom[0] = top[0];
                    bottom[2] = top[2];
                    for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                        k = *list;
                        list++;
                        for (j = 0, quadCorner = quad[0]; j < 4; j++, quadCorner += 3) {
                            corner[0] = *list++;
                            corner[1] = *list++;
                            corner[2] = *list++;
                            Vec_Rotate(rotated, corner, bones[k].world);
                            quadCorner[0] = rotated[0] + bones[k].world[12];
                            quadCorner[1] = rotated[1] + bones[k].world[13];
                            quadCorner[2] = rotated[2] + bones[k].world[14];
                        }
                        Vec_Copy(tri[0], quad[0]);
                        Vec_Copy(tri[1], quad[1]);
                        Vec_Copy(tri[2], quad[2]);
                        if (Tri_SegmentHit(tri[0], top, bottom, corner) == 0) {
                            Vec_Copy(tri[0], quad[1]);
                            Vec_Copy(tri[1], quad[2]);
                            Vec_Copy(tri[2], quad[3]);
                            if (Tri_SegmentHit(tri[0], top, bottom, corner) == 0) {
                                continue;
                            }
                        }
                        hits++;
                        if ((pl->rideState == 1 || pl->rideState == 3) && pl->rideBone == (u8)k) {
                            pl->pos[0] = corner[0];
                            pl->pos[1] = corner[1];
                            pl->pos[2] = corner[2];
                            break;
                        }
                        pl->pos[0] = corner[0];
                        pl->pos[1] = corner[1];
                        pl->pos[2] = corner[2];
                        pl->rideEm = em->slotNo;
                        pl->rideBone = k;
                        if (pl->rideState != 1 && pl->rideState != 3) {
                            pl->action = PL_POSTURE_STANDING;
                            if (pl->vel[1] > 0.0f && pl->stateGroup == PL_GROUP_ACTION && (pl->state == PL_ACT_LEAP_OFF_LEDGE || pl->state == PL_ACT_JUMP_TO_LEDGE)) {
                                pl->rideState = 3;
                                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_HANG, 0);
                            } else {
                                pl->rideState = 1;
                                Player_ReturnToIdle(pl, 0, 0, 0);
                            }
                            pl->weaponOut = 0;
                            Player_StopMove(pl);
                        }
                        break;
                    }
                    if (hits == 0 && pl->rideState != 2) {
                        pl->rideState = 0;
                    }
                } else {
                    pl->rideState = 0;
                    hits++;
                }
            }
        }
    }
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
EmBone *Em_Bone(void *obj, s16 bone);
extern f32 *gEmRideQuads[];

/* The first hit of the vertical segment from 200 above to 200 below point on the ridden monsters'
 * quads (gEmRideQuads[kind]: a bone, four corners, ..., -1): returns 1 with the monster, the bone and
 * the point in the bone's frame; 0 when none. */
s32 Em_HitQuadsVertical(f32 *point, Em **hitEm, s16 *hitBone, f32 *hit)
{
    f32 top[3];
    f32 bottom[3];
    f32 corner[3];
    f32 quad[4][3];
    f32 boneMtx[16];
    f32 inv[16];
    f32 tri[3][3];
    Em *em;
    s16 i;
    f32 *list;
    s16 j;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG || em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
            if ((u8)(s32)Player_InCurrentArea(em)) {
                top[1] = 200.0f + point[1];
                bottom[1] = point[1] - 200.0f;
                top[0] = bottom[0] = point[0];
                top[2] = bottom[2] = point[2];
                for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                    *hitBone = *list;
                    list++;
                    Mtx_Copy(boneMtx, (f32 *)Em_Bone(em, *hitBone));
                    for (j = 0; j < 4; j++) {
                        corner[0] = *list++;
                        corner[1] = *list++;
                        corner[2] = *list++;
                        Vec_RotateSelf(corner, boneMtx);
                        quad[j][0] = corner[0] + boneMtx[12];
                        quad[j][1] = corner[1] + boneMtx[13];
                        quad[j][2] = corner[2] + boneMtx[14];
                    }
                    Vec_Copy(tri[0], quad[0]);
                    Vec_Copy(tri[1], quad[1]);
                    Vec_Copy(tri[2], quad[2]);
                    if (Tri_SegmentHit(tri[0], top, bottom, hit)) {
                        hit[0] -= boneMtx[12];
                        hit[1] -= boneMtx[13];
                        hit[2] -= boneMtx[14];
                        Mtx_Inverse(inv, boneMtx);
                        Vec_RotateSelf(hit, inv);
                        *hitEm = em;
                        return 1;
                    }
                    Vec_Copy(tri[0], quad[1]);
                    Vec_Copy(tri[1], quad[2]);
                    Vec_Copy(tri[2], quad[3]);
                    if (Tri_SegmentHit(tri[0], top, bottom, hit)) {
                        hit[0] -= boneMtx[12];
                        hit[1] -= boneMtx[13];
                        hit[2] -= boneMtx[14];
                        Mtx_Inverse(inv, boneMtx);
                        Vec_RotateSelf(hit, inv);
                        *hitEm = em;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

extern Em gEms[];
void Em_DrawParts(DrawSlot *a);

/* Runs each active monster's update (its handlers), then gives it a draw slot that Em_DrawParts
 * draws. */
void Em_RunAll(void)
{
    s16 i;
    Em *em;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            em->handlers->update(em);
            em->drawSlotId = DrawSlot_Alloc();
            if (em->drawSlotId != -1) {
                em->drawSlot = DrawSlot_Get(em->drawSlotId);
                em->drawSlot->owner = (void *)em;
                em->drawSlot->draw = Em_DrawParts;
            }
        }
    }
}

extern u8 gEmMotionGroupCounts[];

/* The number of motion groups of monster kind i (gEmMotionGroupCounts). */
u8 EmKind_MotionGroupCount(s16 i)
{
    return gEmMotionGroupCounts[i];
}

/* The monster slot (0-3) whose kind (gScene.emKind) is k; -1 when none. The loop steps a byte
 * pointer through the scene and reads emKind[0] from it, which is emKind[i]: indexing emKind
 * compiles to other instructions than the game's. */
s16 EmSlot_FindKind(s16 k)
{
    s16 i;
    u8 *scene;

    for (i = 0, scene = (u8 *)&gScene; i < 4; i++, scene++) {
        if (((SceneState *)scene)->emKind[0] == k) {
            return i;
        }
    }
    return -1;
}

/* The emitter's update: its state (0 start, 1 run, 2 end, 3 free) picks the step. */
void SparkFx_Update(SparkEmitter *emitter)
{
    u8 state;

    state = emitter->state;
    switch (state) {
    case 0:
        SparkFx_Start(emitter);
        break;
    case 1:
        SparkFx_Run(emitter);
        break;
    case 2:
        SparkFx_End(emitter);
        break;
    case 3:
        SparkFx_Free(emitter);
        break;
    }
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

/* The emitter's sound, by its type: only type 7 has one. */
void SparkFx_PlaySe(SparkEmitter *emitter)
{
    switch (emitter->type) {
    case 7:
        Sound_PlaySeAtPosChecked(1, 0x2C, 0, emitter->pos, 1, 0);
        break;
    }
}

extern s16 gSparkFxCounts[];
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
f32 SparkFx_RandSpread(void);
void Spark_Draw(DrawSlot *slot);

/* The emitter's first state: sets up its sparks (gSparkFxCounts by type), each with a draw slot that
 * Spark_Draw draws, their delays, spins and spreads by type; type 6 keeps its height for the sparks
 * and moves to the origin (it follows its monster's bone), type 0 starts 20 higher. Plays the
 * emitter's sound. */
void SparkFx_Start(SparkEmitter *emitter)
{
    Spark *spark = emitter->sparks;
    f32 height;
    s16 n;
    s16 i;

    emitter->state++;
    emitter->visible = 1;
    emitter->draw = NULL;
    emitter->timer = 0;
    n = gSparkFxCounts[emitter->type];
    switch (emitter->type) {
    case 0:
        emitter->pos[1] += 20.0f;
        break;
    case 6:
        height = emitter->pos[1];
        emitter->pos[0] = 0.0f;
        emitter->pos[1] = 0.0f;
        emitter->pos[2] = 0.0f;
        break;
    }
    for (i = 0; i < n; i++, spark++) {
        spark->id = DrawSlot_Alloc();
        spark->variant = i;
        spark->age = 0;
        spark->scale = emitter->scale;
        spark->fixedColor = 0;
        spark->vel[0] = 0.0f;
        spark->vel[1] = 0.0f;
        spark->vel[2] = 0.0f;
        switch (emitter->type) {
        case 0:
            spark->spin = Rand_Next(1);
            spark->spinSpeed = 0;
            break;
        case 1:
        case 5:
            spark->variant = Rand_Next(1) & 7;
            spark->age = -(i & 1) * 4;
            spark->spin = 0;
            spark->spinSpeed = 0;
            spark->vel[0] = i * (10.0f * emitter->scale * SparkFx_RandSpread());
            spark->vel[1] = 0.0f;
            spark->vel[2] = i * (10.0f * emitter->scale * SparkFx_RandSpread());
            break;
        case 2:
            spark->variant = Rand_Next(1) & 7;
            spark->spin = 0;
            spark->spinSpeed = 0;
            break;
        case 3:
        case 4:
        case 7:
            spark->spin = Rand_Next(1);
            spark->spinSpeed = (u8)Rand_Next(1) - 0x80;
            break;
        case 6:
            spark->age = -i * 2;
            spark->vel[1] = height;
            spark->variant = Rand_Next(1) & 7;
            spark->spin = 0;
            spark->spinSpeed = 0;
            break;
        }
        if (spark->id != -1) {
            spark->obj = DrawSlot_Get(spark->id);
            spark->obj->owner = emitter;
            spark->obj->part = i;
            spark->obj->draw = Spark_Draw;
        } else {
            spark->obj = NULL;
        }
    }
    SparkFx_PlaySe(emitter);
}

extern s16 gSparkFxLives[];
extern s16 gSparkFxSteps[];
extern s16 gSparkFxFirstKeys[];
extern f32 *gSparkFxKeys[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* The emitter's run state: until its life is over (gSparkFxLives by type), its sparks start one after
 * another (age from below 0), follow their key tracks (gSparkFxKeys from gSparkFxFirstKeys[type],
 * gSparkFxSteps apart), spin, and are drawn at the emitter's position plus their offset. A spark
 * lives 20 frames for types 1 and 5, 10 for type 6 and the emitter's life otherwise; type 0's fall,
 * and type 6's take the position of bone 2 of the monster the emitter follows. */
void SparkFx_Run(SparkEmitter *emitter)
{
    s16 n = gSparkFxCounts[emitter->type];
    Spark *spark = emitter->sparks;
    u16 life = gSparkFxLives[emitter->type];
    s16 i;
    s16 key = gSparkFxFirstKeys[emitter->type];
    s16 step = gSparkFxSteps[emitter->type];
    f32 bonePos[3];
    f32 *track;
    u16 sparkLife;

    switch (emitter->type) {
    default:
        sparkLife = life;
        break;
    case 1:
    case 5:
        sparkLife = 20;
        break;
    case 6:
        sparkLife = 10;
        break;
    }
    if (++emitter->timer > life) {
        emitter->state++;
        emitter->visible = 0;
        return;
    }

    for (i = 0; i < n; i++) {
        switch (emitter->type) {
        case 1:
        case 5:
        case 6:
            key = gSparkFxFirstKeys[emitter->type];
            break;
        }
        if (++spark->age <= 0) {
            key += step;
            spark++;
            continue;
        }
        if (spark->age == 1) {
            if (emitter->type == 6) {
                if (emitter->owner->active) {
                    Em_GetBonePos(emitter->owner, 2, bonePos);
                    spark->vel[0] = bonePos[0];
                    spark->vel[2] = bonePos[2];
                } else {
                    key += step;
                    spark++;
                    continue;
                }
            }
        } else if (spark->age > sparkLife) {
            key += step;
            spark++;
            continue;
        }
        track = gSparkFxKeys[key++];
        Key_LerpVec3(track, spark->size, spark->age);
        if (spark->fixedColor == 0) {
            track = gSparkFxKeys[key++];
            Key_LerpFloat(track, &spark->alpha, spark->age);
        }
        spark->spin += (s32)spark->spinSpeed;
        switch (emitter->type) {
        case 0:
            if (spark->age > 4) {
                spark->vel[1] -= 10.0f / (sparkLife - 4);
            }
            break;
        }
        if (spark->obj != NULL) {
            spark->obj->pos[0] = emitter->pos[0] + spark->vel[0];
            spark->obj->pos[1] = emitter->pos[1] + spark->vel[1];
            spark->obj->pos[2] = emitter->pos[2] + spark->vel[2];
            OT_AddByDepth(gDrawOtSorted, spark->obj, 0x40, 0);
        }
        spark++;
    }
}

/* The emitter's end state: frees its sparks' draw slots. */
void SparkFx_End(SparkEmitter *emitter)
{
    s16 n;
    Spark *spark;
    s16 i;

    spark = emitter->sparks;
    emitter->state++;
    n = gSparkFxCounts[emitter->type];
    for (i = 0; i < n; i++, spark++) {
        if (spark->obj != NULL) {
            DrawSlot_Free(spark->id);
        }
    }
}

/* The emitter's last state: frees the emitter. */
void SparkFx_Free(SparkEmitter *emitter)
{
    StageObj_Free((struct StageObj *)emitter);
}

void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamMatrix[];

/* Draws a spark (its draw slot's draw): a model of the common set 0 picked by the emitter's type
 * (texture ids 0x5E, 0x1A, 0x5D), scaled by the spark's size, spun, and placed at the slot, facing
 * the camera for types 0-2, 5 and 6, with a texture offset by the spark's variant; tinted by the
 * emitter (white, orange, beige) and the spark's alpha, or the spark's fixed colour. The depth
 * buffer isn't written meanwhile. */
void Spark_Draw(DrawSlot *slot)
{
    SparkEmitter *emitter = (SparkEmitter *)slot->owner;
    Spark *spark = &emitter->sparks[slot->part];
    Model *models = gCommonModels[0];
    u16 flags = 0;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    void *matList;
    ModelMatSet *matSet;
    u8 red;
    u8 green;
    u8 blue;
    u32 color;

    if (models != NULL && models->active) {
        matList = models->mats;
        scale[0] = spark->scale * spark->size[0];
        scale[1] = spark->scale * spark->size[1];
        scale[2] = spark->scale * spark->size[2];
        Gs_SetState(RS_ZWRITE, 0);
        if (emitter->tint == 0) {
            blue = green = red = 0xFF;
        } else if (emitter->tint == 1) {
            red = 0xFF;
            green = 0x7F;
            blue = 0;
        } else {
            red = 0xD8;
            green = 0xB6;
            blue = 0x8A;
        }
        switch (emitter->type) {
        case 0:
            if (emitter->tint == 1) {
                flags |= 2;
            }
            matSet = &models->matSets[Effect_MapTexId(0x5E)];
            rot[2] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 1:
        case 2:
        case 5:
        case 6:
            if (emitter->tint == 1) {
                flags |= 2;
            }
            matSet = &models->matSets[Effect_MapTexId(0x1A)];
            rot[0] = 0.0f;
            if (spark->variant & 1) {
                rot[1] = 3.1415927f;
            } else {
                rot[1] = 0.0f;
            }
            rot[2] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 6, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Mtx_Translation(uvMtx, 0.125f * (spark->variant >> 1), 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 3:
        case 4:
            matSet = &models->matSets[Effect_MapTexId(0x5D)];
            rot[1] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, mtx);
            if (gScene.inLobby == 0) {
                Mtx_Translation(uvMtx, 0.75f, 0.75f, 0.0f);
            } else {
                Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
            }
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case 7:
            matSet = &models->matSets[Effect_MapTexId(0x5D)];
            rot[1] = 6.2831855f * spark->spin / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, mtx);
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        }
        if (spark->fixedColor == 0) {
            color = ((u8)(255.0f * spark->alpha) << 24) | (red << 16) | (green << 8) | blue;
        } else {
            color = spark->color;
        }
        Effect_DrawModel(matSet, mtx, color, flags, matList);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/* A random spread between -0.512 and 0.511. */
f32 SparkFx_RandSpread(void)
{
    return 0.001f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

/* The file's data: the ridden monsters' quads by kind and the spark effect's tables. */
f32 gEmRideQuadsNone[2] = {
#include "sub_main/00468BA8.inc"
};
f32 gEmRideQuadsLaoShanLung[56] = {
#include "sub_main/00468BB0.inc"
};
f32 gEmRideQuadsShenGaoren[16] = {
#include "sub_main/00468C90.inc"
};
f32 gEmRideQuadsYamaTsukami[484] = {
#include "sub_main/00468CD0.inc"
};
f32 *gEmRideQuads[76] = {
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsLaoShanLung,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsLaoShanLung, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsShenGaoren,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsYamaTsukami, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone, gEmRideQuadsNone,
    gEmRideQuadsNone, gEmRideQuadsNone, NULL, NULL,
};
f32 gSparkFxKeys_0[20] = {
#include "sub_main/00469590.inc"
};
f32 gSparkFxKeys_1[8] = {
#include "sub_main/004695E0.inc"
};
f32 gSparkFxKeys_2[28] = {
#include "sub_main/00469600.inc"
};
f32 gSparkFxKeys_3[8] = {
#include "sub_main/00469670.inc"
};
f32 gSparkFxKeys_4[28] = {
#include "sub_main/00469690.inc"
};
f32 gSparkFxKeys_5[8] = {
#include "sub_main/00469700.inc"
};
f32 gSparkFxKeys_6[20] = {
#include "sub_main/00469720.inc"
};
f32 gSparkFxKeys_7[8] = {
#include "sub_main/00469770.inc"
};
f32 gSparkFxKeys_8[16] = {
#include "sub_main/00469790.inc"
};
f32 gSparkFxKeys_9[8] = {
#include "sub_main/004697D0.inc"
};
f32 gSparkFxKeys_10[24] = {
#include "sub_main/004697F0.inc"
};
f32 gSparkFxKeys_11[8] = {
#include "sub_main/00469850.inc"
};
f32 *gSparkFxKeys[12] = {
    gSparkFxKeys_0, gSparkFxKeys_1, gSparkFxKeys_2, gSparkFxKeys_3,
    gSparkFxKeys_4, gSparkFxKeys_5, gSparkFxKeys_6, gSparkFxKeys_7,
    gSparkFxKeys_8, gSparkFxKeys_9, gSparkFxKeys_10, gSparkFxKeys_11,
};
s16 gSparkFxCounts[8] = {
#include "sub_main/004698A0.inc"
};
s16 gSparkFxLives[8] = {
#include "sub_main/004698B0.inc"
};
s16 gSparkFxSteps[8] = {
#include "sub_main/004698C0.inc"
};
s16 gSparkFxFirstKeys[8] = {
#include "sub_main/004698D0.inc"
};
