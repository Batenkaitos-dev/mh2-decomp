#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "prim.h"
#include "sprite.h"
#include "fxobj.h"
#include "model.h"

/* Built at optimization level 4: Tri_SegmentHit matches only at 4, Fish_BaitMatches_Unk (unk_3E3D00.c)
 * only at 3. */
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
s32 Tri_ContainsPointAngles(f32 *tri, f32 *p)
{
    f32 a[3];
    f32 b[3];
    f32 c[3];
    f32 ab;
    f32 bc;
    f32 ca;

    a[0] = tri[0] - p[0];
    a[1] = tri[1] - p[1];
    a[2] = tri[2] - p[2];
    b[0] = tri[3] - p[0];
    b[1] = tri[4] - p[1];
    b[2] = tri[5] - p[2];
    c[0] = tri[6] - p[0];
    c[1] = tri[7] - p[1];
    c[2] = tri[8] - p[2];
    Vec_Normalize(a);
    Vec_Normalize(b);
    Vec_Normalize(c);
    ab = Vec_Dot(a, b);
    bc = Vec_Dot(b, c);
    ca = Vec_Dot(c, a);
    if (ab > 1.0f) {
        ab = 1.0f;
    }
    if (bc > 1.0f) {
        bc = 1.0f;
    }
    if (ca > 1.0f) {
        ca = 1.0f;
    }
    if (ab < -1.0f) {
        ab = -1.0f;
    }
    if (bc < -1.0f) {
        bc = -1.0f;
    }
    if (ca < -1.0f) {
        ca = -1.0f;
    }
    ab = Math_Acos(ab);
    bc = Math_Acos(bc);
    ca = Math_Acos(ca);
    if ((u16)Math_RadToAngle(ab) + (u16)Math_RadToAngle(bc) + (u16)Math_RadToAngle(ca) >= 0xF000) {
        return 1;
    }
    return 0;
}

void Vec_Cross(f32 *out, f32 *a, f32 *b);

/* Where segment p0-p1 crosses the plane of triangle tri: out, and 1 when the point is inside the
 * triangle and between p0 and p1; 2 when p0 lowered by 30 lies on the plane inside it (out = that). */
s32 Tri_SegmentHit(f32 *tri, f32 *p0, f32 *p1, f32 *out)
{
    f32 a[3];
    f32 b[3];
    f32 n[3];
    f32 e1[3];
    f32 e2[3];
    s32 r = 0;
    f32 *ny;
    f32 *nz;
    f32 d;
    f32 dx;
    f32 dy;
    f32 dz;
    f32 t;

    e1[0] = tri[0] - tri[3];
    e1[1] = tri[1] - tri[4];
    e1[2] = tri[2] - tri[5];
    e2[0] = tri[6] - tri[3];
    e2[1] = tri[7] - tri[4];
    e2[2] = tri[8] - tri[5];
    Vec_Cross(n, e2, e1);
    Vec_Normalize(n);
    ny = &n[1];
    nz = &n[2];
    d = -(n[0] * tri[0]) - *ny * tri[1] - *nz * tri[2];
    dx = p1[0] - p0[0];
    dy = p1[1] - p0[1];
    dz = p1[2] - p0[2];
    if (d + (n[0] * p0[0] + *ny * (p0[1] - 30.0f) + *nz * p0[2]) == 0.0f) {
        out[0] = p0[0];
        out[1] = p0[1] - 30.0f;
        out[2] = p0[2];
        if (Tri_ContainsPointAngles(tri, out)) {
            return 2;
        }
    }
    t = -(d + (n[0] * p0[0] + *ny * p0[1] + *nz * p0[2]));
    t /= n[0] * dx + *ny * dy + *nz * dz;
    out[0] = p0[0] + dx * t;
    out[1] = p0[1] + dy * t;
    out[2] = p0[2] + dz * t;
    a[0] = p0[0] - out[0];
    a[1] = p0[1] - out[1];
    a[2] = p0[2] - out[2];
    b[0] = p1[0] - out[0];
    b[1] = p1[1] - out[1];
    b[2] = p1[2] - out[2];
    Vec_Normalize(a);
    Vec_Normalize(b);
    if (Vec_Dot(b, a) < 0.0f && Tri_ContainsPointAngles(tri, out)) {
        r = 1;
    }
    return r;
}

extern f32 gEmBoneMtxBuf[][16];
extern f32 gEmBonePosBuf[][3];
void Mtx_Copy(f32 *dst, f32 *src);

/* Copies each bone matrix of an active monster to gEmBoneMtxBuf[i] (Mtx_Copy) and its
 * translation to gEmBonePosBuf[i]. */
void Em_CopyBoneMatrices(Em *em)
{
    s16 i;
    EmBone *b;
    s16 n;

    if (em->active == 0 || em->visible == 0) {
        return;
    }
    b = em->model->bones;
    n = b->count;
    for (i = 0; i < n; i++, b++) {
        gEmBonePosBuf[i][0] = b->world[12];
        gEmBonePosBuf[i][1] = b->world[13];
        gEmBonePosBuf[i][2] = b->world[14];
        Mtx_Copy(gEmBoneMtxBuf[i], b->world);
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
void func_849700(Em *em, EmBone *bones);
f32 func_758860(Em *em);
extern EmBendRow *gEmBendOrigins[];

/* Per-kind bone adjustments before the skeleton is updated (Em_UpdateMatrix). */
void Em_AdjustKindBones(Em *em, EmBone *eb)
{
    EmBone *bones = eb;
    f32 m[16];
    f32 t[16];
    EmBendRow *tbl;
    EmFlyerWork *w;
    EmPlesiothWork *fin;
    f32 *p;
    f32 f;
    f32 a;
    f32 b;
    f32 c;

    tbl = gEmBendOrigins[em->type];
    switch (em->type) {
    case 7:
    case 0x32:
    case 1:
    case 0x2A:
    case 0x25:
    case 0xB:
    case 0x29:
    case 0x31:
        Mtx_RotateY(bones[tbl->bendBone[3]].local, Math_Angle16ToRad((u16)em->bendAngle[3]));
        Mtx_RotateY(bones[tbl->bendBone[2]].local, Math_Angle16ToRad((u16)em->bendAngle[2]));
        Mtx_RotateY(bones[tbl->bendBone[1]].local, Math_Angle16ToRad((u16)em->bendAngle[1]));
        Mtx_RotateY(bones[tbl->bendBone[0]].local, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    case 6:
    case 0x26:
    case 0x28:
        w = (EmFlyerWork *)&em->work;
        if (em->combat != 1 || em->hp < (s16)(0.2f * em->hpMax)) {
            if (w->earOpen > 0) {
                w->earOpen--;
            }
        } else {
            if (w->earOpen < 30) {
                w->earOpen++;
            }
        }
        Mtx_Copy(m, Em_BoneMatrixB(em, 0x25));
        Mtx_ClearRotation(m);
        Mtx_RotateX(m, -1.308997f + 0.043633234f * (u32)w->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x25), m);
        Mtx_Copy(m, Em_BoneMatrixB(em, 0x26));
        Mtx_ClearRotation(m);
        Mtx_RotateY(m, 0.5235988f - 0.017453292f * (u32)w->earOpen);
        Mtx_RotateZ(m, 0.87266463f - 0.072722055f * (u32)w->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x26), m);
        Mtx_Copy(m, Em_BoneMatrixB(em, 0x27));
        Mtx_ClearRotation(m);
        Mtx_RotateY(m, -0.5235988f + 0.017453292f * (u32)w->earOpen);
        Mtx_RotateZ(m, -0.87266463f + 0.072722055f * (u32)w->earOpen);
        Mtx_Copy(Em_BoneMatrixB(em, 0x27), m);
        break;
    case 0xF:
    case 0x2D:
        Mtx_Copy(t, Em_BoneMatrixB(em, 0x23));
        Mtx_Transpose3x3(t);
        Mtx_Copy(m, Em_BoneMatrixB(em, 0x25));
        Mtx_CopyRot(m, t);
        Mtx_Copy(Em_BoneMatrixB(em, 0x25), m);
        break;
    case 0x21:
        if (em->stateGroup != 5) {
            Mtx_Copy(m, Em_BoneMatrixB(em, 0x18));
            Mtx_ClearRotation(m);
            Mtx_RotateX(m, (6.2831855f * (f32)-((gScene.frame & 0xF) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 0x18), m);
            Mtx_Copy(m, Em_BoneMatrixB(em, 0x19));
            Mtx_ClearRotation(m);
            Mtx_RotateX(m, (6.2831855f * (f32)-(((gScene.frame & 0xF) - 5) << 12)) / 65536.0f);
            Mtx_CopyRot(Em_BoneMatrixB(em, 0x19), m);
        } else {
            Mtx_ClearRotation(Em_BoneMatrixB(em, 0x18));
            Mtx_ClearRotation(Em_BoneMatrixB(em, 0x19));
        }
        break;
    case 0x15:
    case 0x2E:
    case 8:
    case 0x22:
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
        fin = (EmPlesiothWork *)&em->work;
        Mtx_RotateX(bones[37].local, Math_Angle16ToRad(fin->finRot[0].fold));
        Mtx_RotateX(bones[38].local, Math_Angle16ToRad(fin->finRot[1].fold));
        Mtx_RotateX(bones[39].local, Math_Angle16ToRad(fin->finRot[2].fold));
        Mtx_RotateX(bones[16].local, Math_Angle16ToRad(fin->finRot[3].fold));
        Mtx_RotateY(bones[37].local, Math_Angle16ToRad(fin->finRot[0].wave));
        Mtx_RotateY(bones[38].local, Math_Angle16ToRad(fin->finRot[1].wave));
        Mtx_RotateY(bones[39].local, Math_Angle16ToRad(fin->finRot[2].wave));
        Mtx_RotateY(bones[16].local, Math_Angle16ToRad(fin->finRot[3].wave));
        break;
    case 0x37:
        bones[11].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[0];
        bones[12].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[1];
        bones[13].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[2];
        bones[14].pose[2] = ((EmShenGaorenWork *)&em->work)->ease[3];
        break;
    case 0x38:
        p = Em_BoneMatrixB(em, 2);
        Mtx_ClearRotation(p);
        f = (6.2831855f * (f32)(gScene.frame << 13)) / 65536.0f;
        Mtx_RotateXYZ(p, f, f, f);
        break;
    case 0x3A:
    case 0x48:
        func_849700(em, bones);
        break;
    case 0x43:
        f = func_758860(em);
        if (em->rage == 0) {
            a = 0.9f + 0.1f * f;
            b = 0.85f + 0.15f * f;
            c = 0.75f + 0.25f * f;
            bones[6].pose[0] = a;
            bones[6].pose[1] = b;
            bones[6].pose[2] = c;
            bones[10].pose[0] = a;
            bones[10].pose[1] = b;
            bones[10].pose[2] = c;
            Mtx_ClearRotation(bones[6].local);
            Mtx_ClearRotation(bones[10].local);
            f = (6.2831855f * (170.0f - 170.0f * f)) / 360.0f;
            Mtx_RotateX(bones[6].local, f);
            Mtx_RotateX(bones[10].local, f);
            Mtx_RotateY(bones[6].local, 0.0f);
            Mtx_RotateY(bones[10].local, 0.0f);
            Mtx_RotateZ(bones[6].local, 0.0f);
            Mtx_RotateZ(bones[10].local, 0.0f);
        } else {
            a = 1.0f - 0.1f * f;
            b = 1.0f - 0.15f * f;
            c = 1.0f - 0.25f * f;
            bones[6].pose[0] = a;
            bones[6].pose[1] = b;
            bones[6].pose[2] = c;
            bones[10].pose[0] = a;
            bones[10].pose[1] = b;
            bones[10].pose[2] = c;
            Mtx_ClearRotation(bones[6].local);
            Mtx_ClearRotation(bones[10].local);
            f = (6.2831855f * (170.0f * f)) / 360.0f;
            Mtx_RotateX(bones[6].local, f);
            Mtx_RotateX(bones[10].local, f);
            Mtx_RotateY(bones[6].local, 0.0f);
            Mtx_RotateY(bones[10].local, 0.0f);
            Mtx_RotateZ(bones[6].local, 0.0f);
            Mtx_RotateZ(bones[10].local, 0.0f);
        }
        break;
    case 0x35:
        f = (f32)((EmBlangongaWork *)&em->work)->rageTimer / 15.0f;
        bones[15].pose[0] = 1.0f + 6.0f * f;
        bones[15].pose[1] = 1.0f + 10.0f * f;
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

/* The matrices of bone b and its subtree: scaled by x, y, z and the bone's scale (the children
 * inherit the product), the local matrix, the parent's; bones 0x21-0x24 turn by the monster's
 * angles at +0x648 down to +0x63C. */
void Bone_UpdateTree(Em *em, EmBone *b, f32 *parent, f32 x, f32 y, f32 z)
{
    f32 t[16];
    f32 sx;
    f32 sy;
    f32 sz;

    sx = b->pose[0] * x;
    sy = b->pose[1] * y;
    sz = b->pose[2] * z;
    Mtx_Scale(b->world, sx, sy, sz);
    Mtx_Identity(t);
    t[0] = 1.0f / x;
    t[5] = 1.0f / y;
    t[10] = 1.0f / z;
    t[12] = b->local[12];
    t[13] = b->local[13];
    t[14] = b->local[14];
    Mtx_Concat3x3(b->world, b->local);
    Mtx_Concat(b->world, t);
    Mtx_Concat(b->world, parent);
    switch (b->index) {
    case 0x21:
        Mtx_RotateY(b->world, Math_Angle16ToRad((u16)em->bendAngle[3]));
        break;
    case 0x22:
        Mtx_RotateY(b->world, Math_Angle16ToRad((u16)em->bendAngle[2]));
        break;
    case 0x23:
        Mtx_RotateY(b->world, Math_Angle16ToRad((u16)em->bendAngle[1]));
        break;
    case 0x24:
        Mtx_RotateY(b->world, Math_Angle16ToRad((u16)em->bendAngle[0]));
        break;
    }
    if (b->child != NULL) {
        Bone_UpdateTree(em, b->child, b->world, sx, sy, sz);
    }
    if (b->sibling != NULL) {
        Bone_UpdateTree(em, b->sibling, parent, x, y, z);
    }
}

void Bone_UpdateTree(Em *em, EmBone *b, f32 *parent, f32 x, f32 y, f32 z);

/* Bone_UpdateTree with the scale 1. */
void Bone_UpdateTreeUnscaled(void *p, void *bones, f32 *m)
{
    Bone_UpdateTree(p, bones, m, 1.0f, 1.0f, 1.0f);
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Em_AdjustKindBones(Em *em, EmBone *bones);
void Bone_UpdateWorldScaledRoot(EmBone *bones, f32 *m);

/* The monster's matrix from its scale (+0xB8), angles (+0xA0) and position, stored at +0x60, then its bones (Bone_UpdateTreeUnscaled for kind 0x3B). */
void Em_UpdateMatrix(Em *em)
{
    Model *model = em->model;
    f32 m[16];
    f32 s[16];

    Mtx_Scale(s, em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, m);
    Mtx_SetTrans(m, em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(m, s);
    Mtx_Copy(em->worldMtx[0], m);
    Em_AdjustKindBones(em, model->bones);
    if (em->type == 0x3B) {
        Bone_UpdateTreeUnscaled(em, model->bones, m);
        return;
    }
    Bone_UpdateWorldScaledRoot(model->bones, m);
}

extern f32 gEmBoneMtxBuf[][16];
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* out = obj's position (a monster's, or a player's: the same offset) in the frame of matrix i of
 * gEmBoneMtxBuf. em (the monster carrying obj) is unused. */
void Em_PosInFrame(Em *obj, Em *em, s16 i, f32 *out)
{
    f32 d[3];
    f32 t[3];
    f32 inv[16];
    f32 *m = gEmBoneMtxBuf[i];

    Mtx_Inverse(inv, m);
    Mtx_GetTrans(t, m);
    d[0] = obj->posX - t[0];
    d[1] = obj->posY - t[1];
    d[2] = obj->posZ - t[2];
    Vec_Rotate(out, d, inv);
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
s32 func_849C20(Em *em);
s32 Obj_SameArea(void *obj, Player *b);
s32 Obj_InCurrentArea(void *obj);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_ReturnToIdle(Player *p, s32 a, s32 b, s32 c);
void Player_StopMove(void *obj);
extern f32 *gEmRideQuads[];

/* Players standing on a monster (kinds 7, 0x32, 0x37, 0x3A and 0x48): a rider (Player +0x588 1
 * or 3) is carried along with its bone; then each player in the area is dropped on the monster's
 * hit quads (gEmRideQuads) from above and starts riding it. */
void Em_CarryRiders(Em *em)
{
    f32 v[3];
    f32 t[3];
    f32 old[3];
    f32 r[3];
    f32 a[3];
    f32 b[3];
    f32 tri[3][3];
    f32 quad[4][3];
    Player *p;
    EmBone *bones;
    s32 i;
    s32 hits;
    s32 skip;
    s32 k;
    f32 *list;
    s32 j;
    f32 *q;
    u8 f;

    p = gPlayers;
    f = 0;
    bones = em->model->bones;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    if (em->type == 7 || em->type == 0x32 || em->type == 0x37 || em->type == 0x3A || em->type == 0x48) {
        if ((u8)(s32)Player_InCurrentArea(em)) {
            if ((em->type == 0x3A || em->type == 0x48) && func_849C20(em) == 0) {
                f = 1;
            }
            skip = f;
            for (i = 0; i < 4; i++, p++) {
                if (!(u8)Obj_SameArea(p, (Player *)em) || !(u8)Obj_InCurrentArea(p)) {
                    continue;
                }
                if (p->rideState == 1 || p->rideState == 3) {
                    if (p->rideEm == em->slotNo) {
                        Vec_Copy(old, p->pos);
                        k = p->rideBone;
                        Em_PosInFrame((Em *)p, em, k, t);
                        Vec_Rotate(v, t, bones[k].world);
                        Mtx_GetTrans(t, bones[k].world);
                        p->pos[0] = v[0] + t[0];
                        p->pos[1] = v[1] + t[1];
                        p->pos[2] = v[2] + t[2];
                        p->rideDelta[0] = p->pos[0] - old[0];
                        p->rideDelta[1] = p->pos[1] - old[1];
                        p->rideDelta[2] = p->pos[2] - old[2];
                    }
                }
                if (p->stateGroup != PL_GROUP_REACT && p->stateGroup != PL_GROUP_FAINT && p->stateGroup != PL_GROUP_EVENT) {
                    if (skip) {
                        continue;
                    }
                    if (p->vel[1] > 0.0f && p->stateGroup == PL_GROUP_ACTION && p->state == PL_ACT_LEAP_OFF_LEDGE && (s32)p->motionTime < 0x15 && p->stateVar0 == 0) {
                        continue;
                    }
                    if (p->vel[1] > 0.0f) {
                        a[1] = 230.0f + p->pos[1];
                        b[1] = a[1] - 60.0f;
                    } else {
                        a[1] = 30.0f + p->pos[1];
                        b[1] = a[1] - 60.0f;
                    }
                    a[0] = p->pos[0];
                    hits = 0;
                    a[2] = p->pos[2];
                    b[0] = a[0];
                    b[2] = a[2];
                    for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                        k = *list;
                        list++;
                        for (j = 0, q = quad[0]; j < 4; j++, q += 3) {
                            v[0] = *list++;
                            v[1] = *list++;
                            v[2] = *list++;
                            Vec_Rotate(r, v, bones[k].world);
                            q[0] = r[0] + bones[k].world[12];
                            q[1] = r[1] + bones[k].world[13];
                            q[2] = r[2] + bones[k].world[14];
                        }
                        Vec_Copy(tri[0], quad[0]);
                        Vec_Copy(tri[1], quad[1]);
                        Vec_Copy(tri[2], quad[2]);
                        if (Tri_SegmentHit(tri[0], a, b, v) == 0) {
                            Vec_Copy(tri[0], quad[1]);
                            Vec_Copy(tri[1], quad[2]);
                            Vec_Copy(tri[2], quad[3]);
                            if (Tri_SegmentHit(tri[0], a, b, v) == 0) {
                                continue;
                            }
                        }
                        hits++;
                        if ((p->rideState == 1 || p->rideState == 3) && p->rideBone == (u8)k) {
                            p->pos[0] = v[0];
                            p->pos[1] = v[1];
                            p->pos[2] = v[2];
                            break;
                        }
                        p->pos[0] = v[0];
                        p->pos[1] = v[1];
                        p->pos[2] = v[2];
                        p->rideEm = em->slotNo;
                        p->rideBone = k;
                        if (p->rideState != 1 && p->rideState != 3) {
                            p->action = 0;
                            if (p->vel[1] > 0.0f && p->stateGroup == PL_GROUP_ACTION && (p->state == 6 || p->state == 0x15)) {
                                p->rideState = 3;
                                Player_SetState(p, PL_GROUP_ACTION, PL_ACT_LEDGE_HANG, 0);
                            } else {
                                p->rideState = 1;
                                Player_ReturnToIdle(p, 0, 0, 0);
                            }
                            p->weaponOut = 0;
                            Player_StopMove(p);
                        }
                        break;
                    }
                    if (hits == 0 && p->rideState != 2) {
                        p->rideState = 0;
                    }
                } else {
                    p->rideState = 0;
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

/* The first hit of the vertical segment 200 above to 200 below p on the monsters' hit quads
 * (kinds 7, 0x32, 0x37, 0x3A and 0x48; gEmRideQuads[kind]: a bone, four corners, ... -1): 1 with
 * the monster, the bone and the point in the bone's frame; 0 when none. */
s32 Em_HitQuadsVertical(f32 *p, Em **hitEm, s16 *hitBone, f32 *hit)
{
    f32 a[3];
    f32 b[3];
    f32 v[3];
    f32 quad[4][3];
    f32 m[16];
    f32 inv[16];
    f32 tri[3][3];
    Em *em;
    s16 i;
    f32 *list;
    s16 j;

    for (i = 0, em = gEms; i < 20; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (em->type == 7 || em->type == 0x32 || em->type == 0x37 || em->type == 0x3A || em->type == 0x48) {
            if ((u8)(s32)Player_InCurrentArea(em)) {
                a[1] = 200.0f + p[1];
                b[1] = p[1] - 200.0f;
                a[0] = b[0] = p[0];
                a[2] = b[2] = p[2];
                for (list = gEmRideQuads[em->type]; *list != -1.0f; ) {
                    *hitBone = *list;
                    list++;
                    Mtx_Copy(m, (f32 *)Em_Bone(em, *hitBone));
                    for (j = 0; j < 4; j++) {
                        v[0] = *list++;
                        v[1] = *list++;
                        v[2] = *list++;
                        Vec_RotateSelf(v, m);
                        quad[j][0] = v[0] + m[12];
                        quad[j][1] = v[1] + m[13];
                        quad[j][2] = v[2] + m[14];
                    }
                    Vec_Copy(tri[0], quad[0]);
                    Vec_Copy(tri[1], quad[1]);
                    Vec_Copy(tri[2], quad[2]);
                    if (Tri_SegmentHit(tri[0], a, b, hit)) {
                        hit[0] -= m[12];
                        hit[1] -= m[13];
                        hit[2] -= m[14];
                        Mtx_Inverse(inv, m);
                        Vec_RotateSelf(hit, inv);
                        *hitEm = em;
                        return 1;
                    }
                    Vec_Copy(tri[0], quad[1]);
                    Vec_Copy(tri[1], quad[2]);
                    Vec_Copy(tri[2], quad[3]);
                    if (Tri_SegmentHit(tri[0], a, b, hit)) {
                        hit[0] -= m[12];
                        hit[1] -= m[13];
                        hit[2] -= m[14];
                        Mtx_Inverse(inv, m);
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
void Obj_DrawParts(DrawSlot *a);

/* The draw slot of a monster: Em +0x52C and its id at +0x530. */
typedef struct EmDrawSlot {
    u8 pad[0x52C];
    DrawSlot *slot;             /* 0x52C */
    s16 slotId;                 /* 0x530 */
} EmDrawSlot;

/* For each active monster: entry 1 of its function table, then a draw slot drawn by Obj_DrawParts. */
void Em_RunAll(void)
{
    s16 i;
    Em *em;

    for (i = 0, em = gEms; i < 20; i++, em++) {
        if (em->active != 0) {
            EmDrawSlot *d = (EmDrawSlot *)em;

            em->handlers->update(em);
            d->slotId = DrawSlot_Alloc();
            if (d->slotId != -1) {
                d->slot = DrawSlot_Get(d->slotId);
                d->slot->owner = (void *)em;
                d->slot->draw = Obj_DrawParts;
            }
        }
    }
}

extern u8 gEmMotionGroupCounts[];

u8 EmKind_MotionGroupCount(s16 i)
{
    return gEmMotionGroupCounts[i];
}

/* The player slot (0-3) whose SceneState.emKind byte is k; -1 when none. */
s16 EmSlot_FindKind(s16 k)
{
    s16 i;
    u8 *p;

    for (i = 0, p = (u8 *)&gScene; i < 4; i++, p++) {
        if (p[0x2A] == k) {
            return i;
        }
    }
    return -1;
}

void SparkFx_Update(SparkEmitter *e)
{
    u8 state;

    state = e->state;
    switch (state) {
    case 0:
        SparkFx_Start(e);
        break;
    case 1:
        SparkFx_Run(e);
        break;
    case 2:
        SparkFx_End(e);
        break;
    case 3:
        SparkFx_Free(e);
        break;
    }
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

void SparkFx_PlaySe(SparkEmitter *e)
{
    switch (e->type) {
    case 7:
        Sound_PlaySeAtPosChecked(1, 0x2C, 0, e->pos, 1, 0);
        break;
    }
}

extern s16 gSparkFxCounts[];
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
f32 SparkFx_RandSpread(void);
void Spark_Draw(DrawSlot *slot);

void SparkFx_Start(SparkEmitter *e)
{
    Spark *p = e->sparks;
    f32 y;
    s16 n;
    s16 i;

    e->state++;
    e->visible = 1;
    e->unk14 = 0;
    e->timer = 0;
    n = gSparkFxCounts[e->type];
    switch (e->type) {
    case 0:
        e->pos[1] += 20.0f;
        break;
    case 6:
        y = e->pos[1];
        e->pos[0] = 0.0f;
        e->pos[1] = 0.0f;
        e->pos[2] = 0.0f;
        break;
    }
    for (i = 0; i < n; i++, p++) {
        p->id = DrawSlot_Alloc();
        p->unk2 = i;
        p->unk2C = 0;
        p->unk24 = e->scale;
        p->unk2E = 0;
        p->vel[0] = 0.0f;
        p->vel[1] = 0.0f;
        p->vel[2] = 0.0f;
        switch (e->type) {
        case 0:
            p->unk20 = Rand_Next(1);
            p->unk22 = 0;
            break;
        case 1:
        case 5:
            p->unk2 = Rand_Next(1) & 7;
            p->unk2C = -(i & 1) * 4;
            p->unk20 = 0;
            p->unk22 = 0;
            p->vel[0] = i * (10.0f * e->scale * SparkFx_RandSpread());
            p->vel[1] = 0.0f;
            p->vel[2] = i * (10.0f * e->scale * SparkFx_RandSpread());
            break;
        case 2:
            p->unk2 = Rand_Next(1) & 7;
            p->unk20 = 0;
            p->unk22 = 0;
            break;
        case 3:
        case 4:
        case 7:
            p->unk20 = Rand_Next(1);
            p->unk22 = (u8)Rand_Next(1) - 0x80;
            break;
        case 6:
            p->unk2C = -i * 2;
            p->vel[1] = y;
            p->unk2 = Rand_Next(1) & 7;
            p->unk20 = 0;
            p->unk22 = 0;
            break;
        }
        if (p->id != -1) {
            p->obj = DrawSlot_Get(p->id);
            p->obj->owner = e;
            p->obj->part = i;
            p->obj->draw = Spark_Draw;
        } else {
            p->obj = NULL;
        }
    }
    SparkFx_PlaySe(e);
}

extern s16 gSparkFxLives[];
extern s16 gSparkFxSteps[];
extern s16 D_004698D0[];
extern f32 *gSparkFxKeys[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* The emitter run state: until its duration (gSparkFxLives by type) its sparks, which live 20 frames
 * for types 1 and 5, 10 for 6 and the duration otherwise,
 * start one after another, follow their key tracks (gSparkFxKeys) and are drawn at the emitter
 * position plus their offset; type 6 follows bone 2 of its monster. */
void SparkFx_Run(SparkEmitter *e)
{
    s16 n = gSparkFxCounts[e->type];
    Spark *p = e->sparks;
    u16 life = gSparkFxLives[e->type];
    s16 i;
    s16 k = D_004698D0[e->type];
    s16 step = gSparkFxSteps[e->type];
    f32 v[3];
    f32 *track;
    u16 lim;

    switch (e->type) {
    default:
        lim = life;
        break;
    case 1:
    case 5:
        lim = 20;
        break;
    case 6:
        lim = 10;
        break;
    }
    if (++e->timer > life) {
        e->state++;
        e->visible = 0;
        return;
    }

    for (i = 0; i < n; i++) {
        switch (e->type) {
        case 1:
        case 5:
        case 6:
            k = D_004698D0[e->type];
            break;
        }
        if (++p->unk2C <= 0) {
            k += step;
            p++;
            continue;
        }
        if (p->unk2C == 1) {
            if (e->type == 6) {
                if (e->owner->active) {
                    Em_GetBonePos(e->owner, 2, v);
                    p->vel[0] = v[0];
                    p->vel[2] = v[2];
                } else {
                    k += step;
                    p++;
                    continue;
                }
            }
        } else if (p->unk2C > lim) {
            k += step;
            p++;
            continue;
        }
        track = gSparkFxKeys[k++];
        Key_LerpVec3(track, p->unk4, p->unk2C);
        if (p->unk2E == 0) {
            track = gSparkFxKeys[k++];
            Key_LerpFloat(track, &p->unk30, p->unk2C);
        }
        p->unk20 += (s32)p->unk22;
        switch (e->type) {
        case 0:
            if (p->unk2C > 4) {
                p->vel[1] -= 10.0f / (lim - 4);
            }
            break;
        }
        if (p->obj != NULL) {
            p->obj->pos[0] = e->pos[0] + p->vel[0];
            p->obj->pos[1] = e->pos[1] + p->vel[1];
            p->obj->pos[2] = e->pos[2] + p->vel[2];
            OT_AddByDepth(gDrawOtSorted, p->obj, 0x40, 0);
        }
        p++;
    }
}

void SparkFx_End(SparkEmitter *e)
{
    s16 n;
    Spark *p;
    s16 i;

    p = e->sparks;
    e->state++;
    n = gSparkFxCounts[e->type];
    for (i = 0; i < n; i++, p++) {
        if (p->obj != NULL) {
            DrawSlot_Free(p->id);
        }
    }
}

void SparkFx_Free(SparkEmitter *e)
{
    StageObj_Free((struct StageObj *)e);
}

void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamMatrix[];

/* Draws a spark (its draw slot's update): a model of set 0 by the emitter's type (0x5E, 0x1A,
 * 0x5D), scaled, turned and placed at the slot, facing the camera for types 0-2, 5 and 6, with a
 * texture offset; tinted by the emitter's +7 (white, orange, beige) and the spark's alpha. */
void Spark_Draw(DrawSlot *slot)
{
    SparkEmitter *e = (SparkEmitter *)slot->owner;
    Spark *p = &e->sparks[slot->part];
    Model *set = gCommonModels[0];
    u16 flags = 0;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    f32 t[16];
    void *base;
    ModelMatSet *mdl;
    u8 r;
    u8 g;
    u8 b;
    u32 color;

    if (set != NULL && set->active) {
        base = set->mats;
        scale[0] = p->unk24 * p->unk4[0];
        scale[1] = p->unk24 * p->unk4[1];
        scale[2] = p->unk24 * p->unk4[2];
        Gs_SetState(0x6C, 0);
        if (e->tint == 0) {
            b = g = r = 0xFF;
        } else if (e->tint == 1) {
            r = 0xFF;
            g = 0x7F;
            b = 0;
        } else {
            r = 0xD8;
            g = 0xB6;
            b = 0x8A;
        }
        switch (e->type) {
        case 0:
            if (e->tint == 1) {
                flags |= 2;
            }
            mdl = &set->matSets[Effect_MapTexId(0x5E)];
            rot[2] = 6.2831855f * p->unk20 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (u32)t);
            break;
        case 1:
        case 2:
        case 5:
        case 6:
            if (e->tint == 1) {
                flags |= 2;
            }
            mdl = &set->matSets[Effect_MapTexId(0x1A)];
            rot[0] = 0.0f;
            if (p->unk2 & 1) {
                rot[1] = 3.1415927f;
            } else {
                rot[1] = 0.0f;
            }
            rot[2] = 6.2831855f * p->unk20 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 6, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Mtx_Translation(t, 0.125f * (p->unk2 >> 1), 0.0f, 0.0f);
            Gs_SetState(0x19, (u32)t);
            break;
        case 3:
        case 4:
            mdl = &set->matSets[Effect_MapTexId(0x5D)];
            rot[1] = 6.2831855f * p->unk20 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, m);
            if (gScene.online == 0) {
                Mtx_Translation(t, 0.75f, 0.75f, 0.0f);
            } else {
                Mtx_Translation(t, 0.25f, 0.0f, 0.0f);
            }
            Gs_SetState(0x19, (u32)t);
            break;
        case 7:
            mdl = &set->matSets[Effect_MapTexId(0x5D)];
            rot[1] = 6.2831855f * p->unk20 / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, 2, m);
            Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (u32)t);
            break;
        }
        if (p->unk2E == 0) {
            color = ((u8)(255.0f * p->unk30) << 24) | (r << 16) | (g << 8) | b;
        } else {
            color = p->color;
        }
        Effect_DrawModel(mdl, m, color, flags, base);
        Gs_SetState(0x6C, 1);
    }
}

f32 SparkFx_RandSpread(void)
{
    return 0.001f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

/* unk_3E0EC0's .data (0x468BA8-0x4698E0) */
f32 gEmRideQuads_0[2] = {
#include "sub_main/00468BA8.inc"
};
f32 gEmRideQuads_7[56] = {
#include "sub_main/00468BB0.inc"
};
f32 gEmRideQuads_55[16] = {
#include "sub_main/00468C90.inc"
};
f32 gEmRideQuads_58[484] = {
#include "sub_main/00468CD0.inc"
};
f32 *gEmRideQuads[76] = {
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_7,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_7, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_55,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_58, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0, gEmRideQuads_0,
    gEmRideQuads_0, gEmRideQuads_0, NULL, NULL,
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
s16 D_004698D0[8] = {
#include "sub_main/004698D0.inc"
};
