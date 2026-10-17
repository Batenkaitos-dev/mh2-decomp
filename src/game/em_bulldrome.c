/*
 * Monster family: Bulldrome (EM_KIND_BULLDROME), from its setup function
 * (EmBulldrome_Setup, called by the per-kind dispatch Em_TypeSetup) to
 * the end of the file (unk_868110.c follows, at 0x868110). Where the
 * family's code ends is not established. See include/em.h.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "em.h"
#include "em_fatalis.h"
#include "objhead.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "area.h"
/* prototypes of callees (round 13) */
u8 Player_InCurrentArea(void *obj);

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

/* A short-lived effect object of this range (StageObj_K0A_Create creates one). */
/* An effect object of the monster: a sub_main stage object (include/stageobj.h). */

/* A particle of an StageObj's buffer (0x38 bytes; StageObj_K00_Run steps nine). */
typedef struct EmFxPart {
    s16 idx;            /* 0x00: the mesh (x 0x8C) */
    s16 slot;           /* 0x02 */
    f32 pos[3];         /* 0x04 */
    f32 vel[3];         /* 0x10 */
    f32 acc[3];         /* 0x1C */
    s32 rot[3];         /* 0x28 */
    DrawSlot *node;     /* 0x34 */
} EmFxPart;

/* The matrices of an StageObj buffer (StageObj_K05_Setup sets up two chains of six). */
typedef struct EmFxMtxBuf {
    f32 m[6][2][16];    /* 0x000 */
    DrawSlot *node[2];  /* 0x300 */
    s16 slot[2];        /* 0x308 */
} EmFxMtxBuf;

/* The buffer of the three-part effect (EmFxTri_Setup sets it up). */
typedef struct EmFxTri {
    f32 m[16];          /* 0x00 */
    f32 ofs[3];         /* 0x40: a node's offset (kind 5) */
    s16 slot[3];        /* 0x4C */
    u8 unk52[0x54 - 0x52];
    DrawSlot *node[3];  /* 0x54 */
} EmFxTri;

/* A segment of an StageObj's buffer (0x18 bytes; EmFxSeg_Setup sets them up). */
typedef struct EmFxSeg {
    s16 idx;            /* 0x00 */
    s16 slot;           /* 0x02 */
    DrawSlot *node;     /* 0x04 */
    f32 pos[3];         /* 0x08 */
    s16 unk14;          /* 0x14 */
    u8 unk16[0x18 - 0x16];
} EmFxSeg;

/* The buffer of the dual blades' segments (EmFxSeg_Setup sets up EmFxSeg_Counts[type], 2). */
typedef struct EmFxSegBuf {
    EmFxSeg seg[2];     /* 0x00 */
    s16 sword;          /* 0x30: the owner's dual sword index (Player_DualSwordIndex): the row of the offset and scale tables */
} EmFxSegBuf;

/* A piece of an StageObj's buffer (0x30 bytes; StageObj_K0A_Setup sets them up). */
typedef struct EmFxPiece {
    s16 slot;           /* 0x00 */
    s16 idx;            /* 0x02 */
    f32 pos[3];         /* 0x04 */
    f32 unk10[3];       /* 0x10 */
    f32 unk1C;          /* 0x1C */
    f32 unk20;          /* 0x20 */
    DrawSlot *node;     /* 0x24 */
    s16 unk28;          /* 0x28 */
    u16 unk2A;          /* 0x2A */
    u16 unk2C;          /* 0x2C */
    u8 unk2E[0x30 - 0x2E];
} EmFxPiece;

/* Per-kind bones of StageObj_K0A_Setup's effect (0x10 bytes each). */
typedef struct EmFxBone {
    s16 bone;           /* 0x00 */
    s16 pad;
    f32 v[3];           /* 0x04: an offset */
} EmFxBone;

/* A spark of an StageObj's buffer (0x3C bytes; EmFxSpark_Setup sets them up by effect kind). */
typedef struct EmFxSpark {
    s16 slot;           /* 0x00 */
    s16 kind;           /* 0x02 */
    f32 pos[3];         /* 0x04 */
    s16 unk10;          /* 0x10: a start delay (negative) */
    u16 rot[3];         /* 0x12 */
    f32 scale;          /* 0x18 */
    DrawSlot *node;     /* 0x1C */
    f32 unk20[3];       /* 0x20 */
    u8 unk2C;           /* 0x2C: 0 draws in the gDrawOt32 list, else in gDrawOtSorted */
    u8 bone;            /* 0x2D: the row of EmFxSpark_Bones it follows (EmFxSpark_PickBones) */
    u8 alpha;           /* 0x2E */
    u8 unk2F;           /* 0x2F */
    f32 unk30[3];       /* 0x30: a velocity (kind 9) */
} EmFxSpark;

struct EmHolder;
/* prototypes */
s32 EmHolder_CreateMine(struct EmHolder *h, s32 unused, f32 *pos);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndAllStatus(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
s8 QuestEm_Appear(Em *em);
u16 Rand_Next(s32 stream);
void Em_MoveRotatedAccel(Em *em, void *ang);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmBulldrome_ChangeState: 8 of 115 words differ, EmBulldrome_SetGroup0State: 1 of 76 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_StartRage(Em *);
void Em_StartPendingRage(Em *);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
void Em_TargetPlayerInRange(Em *);
s32 EmChameleos_IsInvisible(Em *);
void EmBulldrome_SetGroup1State(Em *em, u16 state, u8 mode);
void EmBulldrome_SetGroup2State(Em *em, u16 state, u8 mode);
void EmBulldrome_SetGroup3State(Em *em, u16 state, u8 mode);
void EmFatalis_SetIdleState(Em *em, u16 state, u8 mode);
void EmFatalis_SetMoveState(Em *em, u16 state, u8 mode);
void EmFatalis_SetFlyState(Em *em, u16 state, u8 mode);
void EmFatalis_SetAttackState(Em *em, u16 state, u8 mode);
void StageObj_K00_Setup(StageObj *fx);
void StageObj_K00_Draw(DrawSlot *node);
void StageObj_K00_Run(StageObj *fx);
void StageObj_K00_End(StageObj *fx);
void StageObj_K00_Free(StageObj *fx);
void EmFxSeg_Setup(StageObj *fx);
void EmFxSeg_Move(StageObj *fx);
void EmFxSeg_End(StageObj *fx);
void EmFxSeg_Delete(StageObj *fx);
void EmFxSpark_Setup(StageObj *fx);
void EmFxSpark_Move(StageObj *fx);
void EmFxSpark_End(StageObj *fx);
void EmFxSpark_Delete(StageObj *fx);
void StageObj_K05_Setup(StageObj *fx);
void StageObj_K05_Draw(DrawSlot *node);
void StageObj_K05_Run(StageObj *fx);
void StageObj_K05_End(StageObj *fx);
void StageObj_K05_Free(StageObj *fx);
void EmFxTri_Setup(StageObj *fx);
void EmFxTri_Draw(DrawSlot *node);
u8 EmFxTri_AreaType(Em *em);
void Vec_RotateY(f32 *v, f32 angle);
void EmFxTri_Move(StageObj *fx);
void EmFxTri_End(StageObj *fx);
void EmFxTri_Delete(StageObj *fx);
void StageObj_K09_Setup(StageObj *fx);
void StageObj_K09_FollowBone(Em *em, StageObj *fx, s16 mode);
void StageObj_K09_Run(StageObj *fx);
struct QuestEm *QuestEm_FindForEm(struct Em *em);
void MinePoint_SetPos(s32 slot, f32 *pos);
s32 MinePoint_Count(s32 slot);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void StageObj_K09_Place(StageObj *fx);
void StageObj_K09_CreateMine(StageObj *fx);
void StageObj_K09_End(StageObj *fx);
void StageObj_K09_Free(StageObj *fx);
void StageObj_K0A_Setup(StageObj *fx);
void StageObj_K0A_Run(StageObj *fx);
void StageObj_K0A_End(StageObj *fx);
void StageObj_K0A_Free(StageObj *fx);
void StageObj_K0B_Setup(StageObj *fx);
void StageObj_K0B_Run(StageObj *fx);
void StageObj_K0B_End(StageObj *fx);
void StageObj_K0B_Free(StageObj *fx);
void StageObj_K0C_Setup(StageObj *o);
void StageObj_K0C_Run(StageObj *o);
void StageObj_K0C_SetupType0(StageObj *o);
void StageObj_K0C_RunType0(StageObj *o);
void StageObj_K0C_DrawType0(StageObj *);
void StageObj_K0C_SetupType1(StageObj *o);
void StageObj_K0C_RunType1(StageObj *fx);
void SightBlock_Remove(EmObj *);
extern u16 StageObj_K0C_Angles[];
void StageObj_K0C_DrawType1(StageObj *fx, DrawSlot *node);
void StageObj_K0C_SetupType3(StageObj *o);
void StageObj_K0C_RunType3(StageObj *o);
void StageObj_K0C_DrawType3(StageObj *fx);
void StageObj_K0C_SetupType4(StageObj *o);
void StageObj_K0C_RunType4(StageObj *fx);
void StageObj_K0C_DrawType4(StageObj *fx, DrawSlot *node);
void StageObj_K0C_SetupParts(StageObj *o);
void StageObj_K0C_RunParts(StageObj *o);
void StageObj_K0C_DrawParts(StageObj *o, DrawSlot *node);
void StageObj_K0C_End(StageObj *o);
void StageObj_K0C_Free(StageObj *o);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void StageObj_Free(struct StageObj *p);
void DrawSlot_Free(s32 i);
void DrawSlotB_Free(s32);
u16 Area_DayId(u16 id);
void Versus_MarkDefeat(Em *em);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Effect01_Spawn(void *obj, s16 type);
StageObj *StageObj_K05_Create(void *owner, s32 arg1, s32 kind);
f32 *Em_Bone(void *obj, s32 bone); /* unit's own declaration: with the definition's EmBone *(void *obj, s16 bone) (sub_main/unk_3DDBA0.c), EmFxSpark_BonePos 34 of 35 words differ */
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetTexFilter(s32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateY(f32 *m, f32 a);
s32 Gs_SetState(u8 reg, u32 v);
void Draw_ApplyStates(u32);
void Draw_SetBlendFlags(u16 flags);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Copy(f32 *dst, f32 *src);
extern s16 EmFxSeg_Counts[];
extern s16 EmFxSpark_Counts[];
extern s16 gStageObjK0A_Counts[];
extern s16 gStageObjK0A_Limits[];
extern s16 gStageObjK0A_Steps[];
extern s16 gStageObjK0A_Ends[];
extern s16 D_008F5CC0[];
extern s16 *D_008F5CC8[];
extern void *StageObj_K0A_Keys0[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
extern s16 D_008CDB28[];
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Game_IsOnline(void);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_CheckRageByHp(Em *em, s16 n);
f32 Vec_Distance(f32 *a, f32 *b);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_StartGoalAction(Em *em, s32 arg1);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void MinePoint_Remove(s32);
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Vec_Normalize(f32 *);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *, f32 *, f32 *);
void Vec_Sub(f32 *, f32 *, f32 *);
extern f32 gCamPos;
f32 *Mtx_FromAnglesXYZ(s32 *ang, f32 *mtx);
extern f32 EmBulldrome_LaunchVel[];
extern f32 EmBulldrome_LaunchAccel[];
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s16 id); /* unit's own declaration: s16 (the definition has s32); with the definition's type StageObj_K05_Setup by 4 of 144, StageObj_K09_Setup by 4 of 66, StageObj_K0C_SetupType3 by 5 of 52 words differ */
void ObjList_Remove(EmObj *obj);
void StageObj_K0B_Draw(DrawSlot *node);
void Gfx_SetBlend(s32, s32);
s16 EmSlot_FindKind(s16 k);
s16 Player_DualSwordIndex(void *obj);
s32 SightBlock_Add(EmObj *obj);
void EmFxSeg_Draw(DrawSlot *node);
s32 Obj_InCurrentArea(void *obj);
void StageObj_K09_Draw(DrawSlot *node);
s32 StageObj_K09_OwnerInvisible(Em *em);
s32 StageObj_K09_BoneForKind(Em *em);
void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
struct EmBone;
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
struct MtxSlot;
void Bone_StoreMatricesTo(struct MtxSlot *tbl, struct EmBone *b);
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Blend(f32 *dst, f32 *a, f32 *b, f32 wa, f32 wb);
f32 Vec_Length(f32 *v);
extern f32 EmFxTri_Blink0[];
extern f32 EmFxTri_Blink1[];
extern f32 EmFxTri_ScaleKeys2[];
extern u8 EmFxTri_ColorKeys2[];
void Key_LerpVec2(f32 *k, f32 *out, f32 t);
void Mtx_CopyRot(f32 *m, f32 *src);
void Mtx_RotateX(f32 *m, f32 a);
extern f32 StageObj_K05_Weights2[][2];
extern f32 StageObj_K05_Weights3[][3];
extern s16 D_008CDB18[];
void Tex_Prepare(s32 a, s32 b);
struct GmEmPartTbl;        /* gm_sub gm_em.c */
void GmEm_SetMeshAlphas(Em *em, s32 mode, struct GmEmPartTbl *tbl);
void MtxSlot_LoadList(s16 *list, struct MtxSlot *tbl);
extern u8 gBoneMatrixBuf[];
extern u8 gModelPartLists[];
extern u8 StageObj_K09_OfsXKeys[];
extern u8 StageObj_K09_OfsYKeys[];
extern u8 StageObj_K09_OfsZKeys[];
extern u8 StageObj_K09_RotKeys[];
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s16 id); /* unit's own declaration: s16 (the definition has s32); with the definition's type StageObj_K05_Setup by 89 of 146, StageObj_K09_Setup by 24 of 68, StageObj_K0C_SetupType3 by 4 of 52 words differ */
void StageObj_K0E_CreateOnEm(Em *em, s16 k);
u16 Mtx_Heading(f32 *m);
f32 AreaGrid2_GetFloorY(f32 *pos);
typedef struct { f32 x, z; } EmFxOfs;
extern EmFxOfs EmFxSeg_OffsetL[];
extern EmFxOfs EmFxSeg_OffsetR[];
extern EmFxOfs EmFxSeg_ScaleL[];
extern EmFxOfs D_008CCE54[];
extern EmFxOfs EmFxSeg_ScaleR[];
extern EmFxOfs D_008CCF54[];
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_TurnX(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
s32 Motion_ReachedFrame(void *, s32, f32);
typedef struct EmFxBoneOfs {
    s16 bone;           /* 0x00 */
    s16 pad;
    f32 ofs[3];         /* 0x04 */
    f32 scale;          /* 0x10 */
} EmFxBoneOfs;
extern EmFxBoneOfs EmFxSpark_Bones[];
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
void Camera_SetFov(f32 fov);
void Camera_RestoreFov(void);
void Mtx_RotateZ(f32 *m, f32 a);
extern f32 StageObj_K0C_Scales[];
extern u8 StageObj_K0C_Colors[][4];
extern u8 D_008CE761[][4];
extern u8 D_008CE762[][4];
extern u8 StageObj_K0C_Type3R0[][4];
extern u8 D_008CE781[][4];
extern u8 D_008CE782[][4];
extern u8 StageObj_K0C_Type3R1[][4];
extern u8 D_008CE791[][4];
extern u8 D_008CE792[][4];
extern f32 EmFxTri_Offsets3[][3];
extern f32 EmFxTri_Vecs5[][3];
extern f32 StageObj_K05_ScalesA[];
extern f32 StageObj_K05_ScalesB[];
void StageObj_K05_GetMatrix(f32 *m, StageObj *fx, Em *em, s16 alt);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
s32 Sound_PlaySeAtPosChecked(s32, s32, s32, void *, s32, s32);
void EmFxSeg_BoneMatrix(f32 *m, StageObj *fx, Em *em, EmFxSeg *seg, s16 kind);
void EmFxSeg_SideOffset(f32 *out, StageObj *fx, Em *owner, EmFxSeg *seg);
typedef struct { f32 x, y, z; } EmFxVec3;
extern EmFxVec3 StageObj_K05_Offsets[];
extern EmFxVec3 D_008CDAC4[];
extern EmFxVec3 D_008CDAC8[];
extern EmFxVec3 StageObj_K05_OffsetsB[];
extern EmFxVec3 D_008CDB44[];
extern EmFxVec3 D_008CDB48[];
extern EmFxVec3 StageObj_K05_OffsetsC[];
extern EmFxVec3 D_008CDCC4[];
extern EmFxVec3 D_008CDCC8[];
void StageObj_K05_GetOffset(f32 *out, StageObj *fx, void *obj, s16 alt);
void Em_MoveRotated(Em *em, void *src);
extern EmCurveKey *EmFatalis_MoveVelYCurves[];
extern EmFxBone gStageObjK0A_Bones[];
void StageObj_K0A_Draw(DrawSlot *node);
void EmFxSpark_BonePos(f32 *out, Em *em, f32 *ofs, s32 bone);
void EmFxSpark_Draw(DrawSlot *node);
extern s16 EmFxSpark_Durations[];
extern s16 EmFxSpark_BoneIdx[];
extern s16 EmFxSpark_BoneIdx2[];
extern s16 *EmFxSpark_CurveSets[];
extern void *EmFxSpark_Curves[];
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
f32 AreaGrid2_GetFloorYFiltered(f32 *pos);
void EmFxSpark_PickBones(StageObj *fx, EmFxSpark *p);
void EmFxSpark_RestartKind3(StageObj *fx, EmFxSpark *p);
void EmFxSpark_RestartKind8(StageObj *fx, EmFxSpark *p);
void EmFxSpark_SpawnAt(void *src, u8 k, u8 c, f32 f);
f32 Math_Sin(f32 x);
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), StageObj_K09_FollowBone: 31 of 66 words differ */
void Em_BonePos(void *obj, s32 bone, f32 *out);
u8 func_0083BA70(Em *em);
extern u8 EmFxSpark_Curve0[];
extern u8 EmFxSpark_Curve1[];
extern u8 EmFxSpark_Curve2[];
extern u8 EmFxSpark_Curve3[];
extern u8 *EmFxSpark_KindCurves5[];
extern u8 *EmFxSpark_KindCurves1[];
void Mtx_Identity(f32 *m);
void EmFxSpark_FaceCamera(f32 *arg0, void *pos);
/* end of prototypes */

void EmBulldrome_Setup(Em *em)
{
}

void EmBulldrome_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmBulldrome_TableNop(void)
{
}

void EmBulldrome_SetGroup0State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 11:
        Em_BeginStatus2(em);
        break;
    case 12:
        Em_EndStatus2(em);
        break;
    case 14:
        Em_BeginStatus1(em);
        break;
    case 15:
        Em_EndStatus1(em);
        break;
    case 13:
        em->holdTimer2 = 0;
        break;
    case 16:
        Em_EndStatus4(em);
        em->noReact = 1;
        break;
    case 17:
        Em_EndStatus5(em);
        em->noReact = 1;
        break;
    case 18:
        Em_ResetNotice(em);
        break;
    case 19:
        Em_RefillNeed3(em);
        break;
    case 20:
        Em_StartRage(em);
        break;
    case 21:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmBulldrome_SetGroup1State(Em *em, u16 state, u8 mode)
{
    switch ((u16) state) {
    case 2:
        em->timer = 1800;
    case 1:
        Em_ComputeGoal2(em, em->goal);
        Em_SetSpeedScaled(em, 2, 1000.0f, 25.0f);
        break;
    case 9:
    case 0xA:
        Em_ComputeGoal2(em, em->goal);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 1, state, mode);
}

void EmBulldrome_SetGroup2State(Em *em, u16 state, u8 mode)
{
    u8 *w = &em->work;

    switch ((u16) state) {
    case 1:
        Em_ComputeGoal2(em, em->goal);
        break;
    case 2:
        Em_ComputeGoal2(em, em->goal);
        w[0x2B] = 1;
        *(f32 *) (w + 0x24) = Vec_DistanceXZ(&em->posX, em->goal);
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmBulldrome_SetGroup3State(Em *em, u16 state, u8 mode)
{
    u16 s = state;

    switch (s) {
    case 0:
    case 1:
    case 2:
        Em_ComputeGoal2(em, em->goal);
        if (em->targetNo == -1) {
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            em->goalDist = 300.0f + Vec_DistanceXZ(&em->posX, em->goal);
        }
        em->timer = 90;
        if (s == 2) {
            em->timer = 30;
        }
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 3, state, mode);
}

void EmBulldrome_SetGroup4State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 3:
        Em_BeginStatus4(em);
        break;
    case 4:
        Em_BeginStatus5(em);
        break;
    case 5:
        Em_EndAllStatus(em);
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        goto block_5;
    }
    if ((mode == 3) || (mode == 4)) {
        return;
    }
block_5:
    em->motionSpeed = 1.0f;
    switch (group) {
    case 0:
        EmBulldrome_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmBulldrome_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmBulldrome_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmBulldrome_SetGroup3State(em, state, mode);
        return;
    case 4:
        EmBulldrome_SetGroup4State(em, state, mode);
        return;
    case 5:
        if ((state == 2) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    default:
        Em_SetStateSync(em, group, state, mode);
        return;
    }
}

void EmBulldrome_LaunchFromHit(Em *em)
{
    s32 ang[3];
    f32 m[16];

    ang[2] = 0;
    ang[0] = 0;
    ang[1] = em->hitAngle + 0x8000;
    Mtx_FromAnglesXYZ(ang, m);
    Vec_Rotate(&em->velX, EmBulldrome_LaunchVel, m);
    Vec_Copy(&em->accelX, EmBulldrome_LaunchAccel);
    em->stance = 2;
}

void EmBulldrome_SetMotionForSide(Em *em, s16 frame)
{
    s32 row;

    if ((u16) (em->hitAngle - em->rotY) < 0x8000) {
        row = 0x3E;
    } else {
        row = 0x43;
    }
    Em_SetMotion(em, row, 0, frame);
}

/*
 * Starts a state of group 0: FAT_IDLE and FAT_IDLE_ACTION pick one of the
 * two at random (Em.stateRand); the status states begin or end their
 * status.
 */
void EmFatalis_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_IDLE:
    case FAT_IDLE_ACTION:
        if (em->stateRand & 1) {
            state = FAT_IDLE;
        } else {
            state = FAT_IDLE_ACTION;
        }
        break;
    case FAT_IDLE_STATUS2:
        Em_BeginStatus2(em);
        break;
    case FAT_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case FAT_IDLE_STATUS4_END:
        Em_EndStatus4(em);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/* Starts a state of group 1; the walk sets its speed. */
void EmFatalis_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 0:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 1:
    case 2:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Starts a state of group 2. A flight asked for on the ground takes off
 * first (FAT_FLY_TAKE_OFF); the flights set their speed and their timer
 * (300 frames, or from the goal's distance).
 */
void EmFatalis_SetFlyState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_FLY_HOVER:
    case FAT_FLY_HOVER_TURN:
    case FAT_FLY_HOVER_TURN_STOP:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        em->timer = 300;
        break;
    case FAT_FLY_TO_GOAL:
    case FAT_FLY_LEVEL_TO_GOAL:
    case FAT_FLY_CLIMB_TO_GOAL:
    case FAT_FLY_CLIMB_TO_GOAL_NEAR:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = Vec_DistanceXZ(&em->posX, em->goal) / em->velZ;
        if (em->timer < 30) {
            em->timer = 30;
        }
        break;
    case FAT_FLY_RISE:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case FAT_FLY_STRAIGHT_TO_GOAL:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case FAT_FLY_DESCEND:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_RISE_HIGH:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_TAKE_OFF_AND_RISE:
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_TAKE_OFF:
    case FAT_FLY_LAND:
    case FAT_FLY_LAND_AT_HEIGHT:
    case FAT_FLY_TAKE_OFF_HIGH:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Starts a state of group 3. A ground attack asked for in the air lands
 * first (FAT_FLY_LAND) and an air attack on the ground takes off first; the
 * charge and the glide set their speed, the air fireballs their count, the
 * dives their distance (100, or the goal's) and timer.
 */
void EmFatalis_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_ATK_SINGLE_A:
    case FAT_ATK_SINGLE_B:
    case FAT_ATK_FIREBALL:
    case FAT_ATK_FIREBALL_B:
    case FAT_ATK_SWEEP_NO_EFFECT:
    case FAT_ATK_HOP_FIREBALL:
    case FAT_ATK_CHARGE_START:
    case FAT_ATK_SWEEP_A:
    case FAT_ATK_SWEEP_B:
    case FAT_ATK_SWEEP_C:
    case FAT_ATK_SWEEP_D:
    case FAT_ATK_SWEEP_DOUBLE:
    case FAT_ATK_SINGLE_C:
        if (em->remote == 0 && em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case FAT_ATK_CHARGE:
    case FAT_ATK_GLIDE:
        if (em->remote == 0 && em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        Em_SetSpeedScaled(em, 6, 1000.0f, 500.0f);
        break;
    case FAT_ATK_AIR_FIREBALL:
    case FAT_ATK_AIR_FIREBALL_B:
    case FAT_ATK_AIR_FIREBALL_C:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        w->countdown = 1;
        break;
    case FAT_ATK_AIR_FIREBALL_TRIPLE:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        w->countdown = 3;
        break;
    case FAT_ATK_DIVE:
    case FAT_ATK_DIVE_B:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        if (em->curGoalKind == 0) {
            w->diveToGoal = 0;
            w->runDist = 100.0f;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            w->diveToGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            w->runDist = dist;
            em->timer = (s32) dist / 7;
        }
        break;
    case FAT_ATK_BLASTS:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the state; the sync mode says how the change reaches the other
 * players online, and a remote copy drops the owner's modes (EM_SYNC_OWNER,
 * EM_SYNC_OWNER_FLAGGED). Groups 0-3 go through their Set function;
 * FAT_REACT_KNOCK_DOWN ends every status (Em_EndAllStatus) and
 * FAT_REACT_STATUS4 begins status 4.
 */
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmFatalis_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmFatalis_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmFatalis_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmFatalis_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_EVENT:
    case EM_GROUP_DEFEAT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_REACT:
        switch (state) {
        case FAT_REACT_KNOCK_DOWN:
            Em_EndAllStatus(em);
            break;
        case FAT_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

/*
 * Sets velY from the curve EmFatalis_MoveVelYCurves[kind] at the motion
 * frame (unblended motions only) and moves along rotY.
 */
void EmFatalis_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    tbl = EmFatalis_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &tbl[i];
                keyFrame = key->t;
                if (frame > keyFrame && keyFrame != 0.0f) {
                    i++;
                } else if (keyFrame == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

/*
 * Starts the curves of a flight (velY, speed or both, by kind) at the frame
 * the kind gives (134, 70, 60 or 0; the take-off's curves start at frame
 * 134, where FAT_MOT_TAKE_OFF leaves the floor). Stops the monster.
 */
void EmFatalis_StartCurves(Em *em, u8 kind)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
    case 6:
        w->curveVelY = 1;
        w->curveFrame = 0x86;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x46;
        break;
    case 4:
        w->curveSpeed = 1;
        w->curveFrame = 0x46;
        break;
    case 5:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x3C;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve that does nothing. */
static s32 EmFatalis_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmFatalis_VelYCurves[];

/*
 * The velY curve of EmFatalisWork.curveKind (EmFatalis_VelYCurves): sets
 * velY from the key pair around the curve frame. Returns 2 at the curve's
 * end (a key at t 0: velY then falls 1 a frame down to -50; t below 0: velY
 * 0 and the frame back to 0), else 0.
 */
static u8 EmFatalis_CurveVelY(Em *em, EmWork *work)
{
    EmFatalisWork *w = (EmFatalisWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmFatalis_VelYCurves[((EmFatalisWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmFatalis_SpeedCurves[];

/*
 * The speed curve of EmFatalisWork.curveKind (EmFatalis_SpeedCurves): sets
 * velZ from the key pair around the curve frame. Returns 4 at the curve's
 * end, else 0.
 */
static u8 EmFatalis_CurveSpeed(Em *em, EmWork *work)
{
    EmFatalisWork *w = (EmFatalisWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmFatalis_SpeedCurves[((EmFatalisWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves EmFatalis_StartCurves turned on, advances the curve frame
 * by the motion's step, and moves the monster along rotY while airborne.
 * Returns the ended curves' bits (2 velY, 4 speed), 0 while they run.
 */
u8 EmFatalis_UpdateCurves(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmFatalis_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmFatalis_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmFatalis_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

void StageObj_K00_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        StageObj_K00_Setup(fx);
        break;
    case 1:
        StageObj_K00_Run(fx);
        break;
    case 2:
        StageObj_K00_End(fx);
        break;
    case 3:
        StageObj_K00_Free(fx);
        break;
    }
}

void StageObj_K00_Setup(StageObj *fx)
{
    EmFxPart *p = (EmFxPart *) fx->buf;
    s32 i;

    fx->state++;
    fx->var0 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    for (i = 0; i < 9; i++, p++) {
        p->pos[0] = fx->pos[0];
        p->pos[1] = fx->pos[1];
        p->pos[2] = fx->pos[2];
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        switch (fx->type) {
        case 0:
            p->vel[0] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            p->vel[1] = 2.0f * (5.0f + 0.4f * (Rand_Next(1) % 10));
            p->vel[2] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            break;
        case 1:
            p->vel[0] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            p->vel[1] = 2.0f * (1.5f * (10.0f + 0.5f * (Rand_Next(1) % 10)));
            p->vel[2] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            break;
        }
        switch (i % 4) {
        case 0:
            break;
        case 1:
            p->vel[0] *= -1.0f;
            p->vel[2] *= -1.0f;
            break;
        case 2:
            p->vel[0] *= -1.0f;
            break;
        case 3:
            p->vel[2] *= -1.0f;
            break;
        }
        p->acc[0] = 0.0f;
        p->acc[2] = 0.0f;
        p->acc[1] = 2.0f * (-1.0f * (p->vel[1] / 15.0f));
        p->idx = Rand_Next(1) % 5;
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = fx;
            p->node->part = i;
            p->node->draw = StageObj_K00_Draw;
        } else {
            p->node = NULL;
        }
    }
}

void StageObj_K00_Run(StageObj *fx)
{
    EmFxPart *p = (EmFxPart *) fx->buf;
    s32 i;

    if (fx->timer++ >= 30) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    for (i = 0; i < 9; i++, p++) {
        p->pos[0] += p->vel[0];
        p->pos[1] += p->vel[1];
        p->pos[2] += p->vel[2];
        p->vel[0] += p->acc[0];
        p->vel[1] += p->acc[1];
        p->vel[2] += p->acc[2];
        p->rot[0] += 0x200;
        p->rot[1] += 0x240;
        p->rot[2] += 0x300;
        if (p->node != NULL) {
            p->node->pos[0] = p->pos[0];
            p->node->pos[1] = p->pos[1];
            p->node->pos[2] = p->pos[2];
            OT_AddByDepth(gDrawOt32, p->node, 0x20, 0);
        }
    }
}

void StageObj_K00_End(StageObj *fx)
{
    EmFxPart *p = (EmFxPart *) fx->buf;
    s32 i;

    fx->state++;
    for (i = 0; i < 9; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void StageObj_K00_Free(StageObj *fx)
{
    StageObj_Free(fx);
}

void StageObj_K00_Draw(DrawSlot *node)
{
    StageObj *fx = node->owner;
    EmFxPart *p = (EmFxPart *) fx->buf + node->part;
    ModelMat *tex;
    Model *mdl = gCommonModels[0];
    ModelMatSet *mesh;
    f32 m[16];

    if (mdl != NULL && mdl->active != 0) {
        tex = mdl->mats;
        Gfx_SetTexFilter(0);
        Mtx_Translation(m, p->pos[0], p->pos[1], p->pos[2]);
        Mtx_RotateXYZ(m, (6.2831855f * p->rot[0]) / 65536.0f, (6.2831855f * p->rot[1]) / 65536.0f, (6.2831855f * p->rot[2]) / 65536.0f);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, -1);
        if (fx->var2 == 0) {
            mesh = &mdl->matSets[p->idx] + 32;
        } else if (fx->var2 == 1) {
            mesh = &mdl->matSets[43];
        } else {
            mesh = &mdl->matSets[73];
        }
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(tex, mesh);
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags(0x10);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
        Gfx_SetTexFilter(1);
    }
}

void StageObj_K00_Create(void *owner, u8 k, f32 *pos, u8 c)
{
    StageObj *obj;

    if (Player_InCurrentArea(owner) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = 0;
            obj->update = StageObj_K00_Update;
            obj->pos[0] = pos[0];
            obj->pos[1] = pos[1];
            obj->pos[2] = pos[2];
            obj->owner = owner;
            obj->type = k;
            obj->var2 = c;
        }
    }
}

void EmFxSeg_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        EmFxSeg_Setup(fx);
        break;
    case 1:
        EmFxSeg_Move(fx);
        break;
    case 2:
        EmFxSeg_End(fx);
        break;
    case 3:
        EmFxSeg_Delete(fx);
        break;
    }
}

void EmFxSeg_Setup(StageObj *fx)
{
    EmFxSeg *p;
    Em *m;
    s16 n;
    s16 i;

    p = (EmFxSeg *) fx->buf;
    m = fx->owner;
    fx->state++;
    fx->var0 = 0;
    fx->var1 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    n = EmFxSeg_Counts[fx->type];
    if (m->active == 0 || (((EmFxSegBuf *) p)->sword = Player_DualSwordIndex(m)) == -1) {
        fx->visible = 0;
        EmFxSeg_Delete(fx);
        return;
    }
    for (i = 0; i < n; i++, p++) {
        p->idx = i;
        p->unk14 = i & 1;
        p->slot = DrawSlotB_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlotB_Get(p->slot);
            p->node->owner = fx;
            p->node->part = i;
            p->node->draw = EmFxSeg_Draw;
        } else {
            p->node = NULL;
        }
    }
}

void EmFxSeg_Move(StageObj *fx)
{
    EmFxSeg *seg = (EmFxSeg *) fx->buf;
    Em *em = fx->owner;
    s16 n;
    s16 i;
    f32 m[16];
    f32 pos[3];

    if (em->active == 0) {
        fx->visible = 0;
        fx->state++;
        return;
    }
    if (em->visible == 0 || !(Obj_InCurrentArea(em) & 0xFF)) {
        return;
    }
    if (em->stateGroup == 2 || em->stateGroup == 6) {
        return;
    }
    if ((!(em->statusIcons & 0x8000) && (em->motionId[0] != 0x3FB || Motion_ReachedFrame(em, 0, 22.0f))) ||
        (em->motionId[0] == 0x3FA && !Motion_ReachedFrame(em, 0, 32.0f)) || em->motionId[0] == 0x3C ||
        em->motionId[0] == 0x3D) {
        return;
    }
    n = EmFxSeg_Counts[fx->type];
    for (i = 0; i < n; i++, seg++) {
        seg->unk14 ^= 1;
        EmFxSeg_SideOffset(pos, fx, em, seg);
        EmFxSeg_BoneMatrix(m, fx, em, seg, 0);
        Vec_RotateSelf(pos, m);
        seg->pos[0] = m[12] + pos[0];
        seg->pos[1] = m[13] + pos[1];
        seg->pos[2] = m[14] + pos[2];
        if ((seg->unk14 & 1) && seg->node != NULL) {
            seg->node->pos[0] = seg->pos[0];
            seg->node->pos[1] = seg->pos[1];
            seg->node->pos[2] = seg->pos[2];
            OT_AddByDepth(gDrawOtSorted, seg->node, 0x40, 0);
        }
    }
}

void EmFxSeg_End(StageObj *fx)
{
    EmFxSeg *p = (EmFxSeg *) fx->buf;
    s16 n;
    s16 i;

    fx->state++;
    n = EmFxSeg_Counts[fx->type];
    for (i = 0; i < n; i++) { /* p is not stepped: the first segment's slot is tested n times */
        if (p->node != NULL) {
            DrawSlotB_Free(p->slot);
        }
    }
}

void EmFxSeg_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

void EmFxSeg_Draw(DrawSlot *node)
{
    StageObj *fx = node->owner;
    EmFxSeg *seg = (EmFxSeg *) fx->buf + node->part;
    ModelMat *tex;
    Model *mdl = gCommonModels[0];
    Em *em = fx->owner;
    ModelMatSet *mesh;
    f32 m[16];

    if (mdl == NULL || em->visible == 0) {
        return;
    }
    if (mdl->active != 0) {
        tex = mdl->mats;
        EmFxSeg_BoneMatrix(m, fx, em, seg, 1);
        Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        mesh = &mdl->matSets[59];
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x67, 0x33FF3333);
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(tex, mesh);
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags(2);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 0);
    }
}

void EmFxSeg_BoneMatrix(f32 *m, StageObj *fx, Em *em, EmFxSeg *seg, s16 kind)
{
    EmFxSegBuf *buf = (EmFxSegBuf *) fx->buf;
    f32 sx;
    f32 sy;
    f32 sz;

    switch (kind) {
    case 0:
        if (seg->idx == 0) {
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0xE));
        } else {
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0x12));
        }
        break;
    case 1:
        if (seg->idx == 0) {
            sx = EmFxSeg_ScaleL[buf->sword].x;
            sy = D_008CCE54[buf->sword].x;
            sz = 1.0f;
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0xE));
        } else {
            sx = EmFxSeg_ScaleR[buf->sword].x;
            sy = D_008CCF54[buf->sword].x;
            sz = 1.0f;
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0x12));
            Mtx_TurnZ(m, 3.1415927f);
        }
        Mtx_TurnX(m, 1.5707964f);
        m[0] *= sx;
        m[1] *= sx;
        m[2] *= sx;
        m[4] *= sy;
        m[5] *= sy;
        m[6] *= sy;
        m[8] *= sz;
        m[9] *= sz;
        m[10] *= sz;
        break;
    }
}

void EmFxSeg_SideOffset(f32 *out, StageObj *fx, Em *owner, EmFxSeg *seg)
{
    EmFxSegBuf *buf = (EmFxSegBuf *) fx->buf;

    if (seg->idx == 0) {
        out[0] = 10.0f + EmFxSeg_OffsetL[buf->sword].x;
        out[1] = 0.0f;
        out[2] = EmFxSeg_OffsetL[buf->sword].z;
    } else {
        out[0] = -EmFxSeg_OffsetR[buf->sword].x - 10.0f;
        out[1] = 0.0f;
        out[2] = EmFxSeg_OffsetR[buf->sword].z;
    }
}

void EmFxSeg_Spawn(Em *em, u8 k)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 3;
        obj->update = EmFxSeg_Update;
        obj->draw = NULL;
        obj->type = k;
        obj->owner = em;
        obj->ownerIndex = em->index;
        obj->keep = 1;
    }
}

void EmFxSpark_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        EmFxSpark_Setup(fx);
        break;
    case 1:
        EmFxSpark_Move(fx);
        break;
    case 2:
        EmFxSpark_End(fx);
        break;
    case 3:
        EmFxSpark_Delete(fx);
        break;
    }
}

void EmFxSpark_Setup(StageObj *fx)
{
    EmFxSpark *p;
    Em *em;
    s16 n;
    f32 ofs[3];
    f32 size;
    s16 i;

    p = (EmFxSpark *) fx->buf;
    em = fx->owner;
    fx->state++;
    fx->var0 = 0;
    fx->var1 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    n = EmFxSpark_Counts[fx->type];
    switch (fx->type) {
    case 4:
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 100.0f;
        EmFxSpark_BonePos(fx->pos, em, ofs, 0x22);
        break;
    case 9:
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 80.0f;
        EmFxSpark_BonePos(fx->pos, em, ofs, 0x22);
        break;
    }
    for (i = 0; i < n; p++, i++) {
        p->unk10 = 0;
        p->kind = i;
        p->unk20[0] = 1.0f;
        p->unk20[1] = 1.0f;
        p->unk20[2] = 1.0f;
        p->scale = fx->scale;
        switch (fx->type) {
        case 0:
            size = em->sizePct / 100.0f;
            if (i == 0) {
                p->kind = 0;
                p->rot[0] = Rand_Next(1);
                p->rot[1] = Rand_Next(1);
                p->rot[2] = Rand_Next(1);
                p->scale = 6.0f * size;
                p->unk2C = 0;
            } else {
                p->kind = 1;
                p->rot[2] = Rand_Next(1);
                p->scale = 2.0f * size;
                p->unk2C = 1;
            }
            break;
        case 1:
            p->rot[2] = Rand_Next(1);
            p->unk2C = 1;
            Vec_Copy(p->pos, fx->pos);
            break;
        case 2:
            p->unk10 = -2 - i * 2;
            p->rot[2] = Rand_Next(1);
            p->unk2C = 1;
            p->pos[0] = fx->pos[0] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            p->pos[1] = fx->pos[1] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            p->pos[2] = fx->pos[2] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            Vec_Copy(p->pos, fx->pos);
            p->scale *= 1.2f - 0.2f * i;
            p->unk2F = (Rand_Next(1) & 0x1F) + 0x4B;
            break;
        case 3:
            if (i == 4) {
                p->kind = 3;
                p->unk10 = -5;
            }
            p->rot[2] = Rand_Next(1);
            if (p->kind == 2) {
                p->unk2C = 0;
            } else {
                p->unk2C = 1;
            }
            break;
        case 4:
            switch (i) {
            case 0:
                p->rot[0] = 0;
                p->rot[1] = em->rotY;
                break;
            case 1:
                p->rot[0] = 0;
                p->rot[1] = em->rotY;
                p->scale *= 0.75f;
                break;
            case 2:
                p->rot[2] = Rand_Next(1);
                break;
            case 4:
                p->unk10 = -1;
                p->rot[2] = Rand_Next(1);
                break;
            case 5:
                p->unk10 = -4;
                p->rot[1] = em->rotY + 0x8000;
                p->scale *= 0.5f;
                break;
            }
            p->unk2C = 1;
            Vec_Copy(p->pos, fx->pos);
            p->scale *= 2.0f;
            break;
        case 5:
            if (i == 1) {
                p->kind = 0;
                p->unk10 = -2;
            } else if (i == 2) {
                p->kind = 1;
            }
            p->rot[2] = Rand_Next(1);
            p->unk2C = 1;
            Vec_Copy(p->pos, fx->pos);
            break;
        case 6:
            p->unk10 = -i * 3;
            p->rot[2] = Rand_Next(1);
            p->unk2C = 1;
            Vec_Copy(p->pos, fx->pos);
            break;
        case 7:
            p->unk10 = -4;
            Vec_Copy(p->pos, fx->pos);
            switch (p->kind) {
            case 0:
                p->rot[2] = Rand_Next(1);
                break;
            case 2:
                p->rot[1] = Rand_Next(1);
                p->pos[1] += 5.0f;
                break;
            }
            p->unk2C = 1;
            break;
        case 8:
            switch (p->kind) {
            case 0:
                p->rot[0] = Rand_Next(1);
                p->rot[1] = Rand_Next(1);
                p->rot[2] = Rand_Next(1);
                p->scale *= 0.5f;
                p->unk2C = 0;
                break;
            default:
                p->rot[2] = Rand_Next(1);
                p->scale *= 3.0f;
                p->unk2C = 1;
                break;
            }
            break;
        case 9:
            p->unk20[1] *= 0.33f;
            p->unk30[0] = 0;
            p->unk30[1] = 0;
            p->unk30[2] = 0;
            Vec_Copy(p->pos, fx->pos);
            p->pos[0] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            p->pos[1] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            p->pos[2] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            p->unk2C = 1;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = fx;
            p->node->part = i;
            p->node->draw = EmFxSpark_Draw;
        } else {
            p->node = NULL;
        }
    }
    EmFxSpark_Move(fx);
}

void EmFxSpark_Move(StageObj *fx)
{
    s16 i;
    EmFxSpark *p;
    Em *em;
    s16 n;
    s16 dur;
    s16 *curves;
    s16 end;
    s16 idx;
    s16 step;
    s32 k;
    u8 flag;
    f32 ofs[3];
    f32 a;
    f32 f;
    void *curve;

    p = (EmFxSpark *) fx->buf;
    em = fx->owner;
    n = EmFxSpark_Counts[fx->type];
    dur = EmFxSpark_Durations[fx->type];
    curves = EmFxSpark_CurveSets[fx->type];
    i = EmFxSpark_BoneIdx2[fx->type];
    idx = EmFxSpark_BoneIdx[fx->type];
    step = i;
    if (fx->type == 3 || fx->type == 8) {
        dur = (u16) fx->angle;
    }
    if (++fx->timer > dur) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    switch (fx->type) {
    case 0:
        if (em->active == 0) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x45A) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        EmFxSpark_PickBones(fx, p);
        break;
    case 2:
        end = 0x16;
        break;
    case 3:
        if (em->active == 0) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x417) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 100.0f;
        EmFxSpark_BonePos(fx->pos, em, ofs, 0x22);
        break;
    case 7:
        if (fx->timer == dur) {
            Effect0D_SpawnAt(fx->pos, 0x1B, 1.0f);
        }
        break;
    case 8:
        if (em->active == 0) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x40F) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        ofs[0] = 0.0f;
        ofs[1] = 14.0f;
        ofs[2] = 24.0f;
        EmFxSpark_BonePos(fx->pos, em, ofs, 0x17);
        break;
    case 9:
        for (i = 0; i < n; i++) {
            if (p[i].kind != 0xFF) {
                break;
            }
        }
        if (i >= n) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        break;
    }
    for (i = 0; i < n; i++, p++) {
        p->unk10++;
        if (curves != NULL) {
            end = curves[i];
        } else {
            idx = EmFxSpark_BoneIdx[fx->type];
        }
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 0.0f;
        switch (fx->type) {
        case 0:
            if (p->kind == 0) {
                if (!EmMotion_ReachedFrame(em, 0, 115.0f)) {
                    flag = 1;
                    continue;
                }
                flag = 0;
                if (p->unk10 >= 4) {
                    p->unk10 = 0;
                    p->rot[0] = Rand_Next(1);
                    p->rot[1] = Rand_Next(1);
                    p->rot[2] = Rand_Next(1);
                }
                EmFxSpark_BonePos(p->pos, em, ofs, 3);
            } else if (flag) {
                p->unk2C = 2;
            } else {
                p->unk2C = 1;
            }
            break;
        case 1:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            k = idx;
            Key_LerpVec3(EmFxSpark_Curves[k], p->unk20, p->unk10);
            idx += 2;
            Key_LerpFloat(EmFxSpark_Curves[(s16) (k + 1)], &a, p->unk10);
            p->alpha = 255.0f * a;
            if (p->kind == 0) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = 20.0f;
                Vec_RotateSelf(ofs, gCamMatrix);
            } else {
                p->pos[1] -= 50.0f / end;
            }
            break;
        case 2:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            k = idx;
            Key_LerpVec3(EmFxSpark_Curves[k], p->unk20, p->unk10);
            idx += 2;
            Key_LerpFloat(EmFxSpark_Curves[(s16) (k + 1)], &a, p->unk10);
            p->alpha = 255.0f * a;
            p->pos[1] += fx->scale * (p->unk2F / (f32) end);
            break;
        case 3:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                EmFxSpark_RestartKind3(fx, p);
                if (p->unk10 <= 0) {
                    idx += step;
                    continue;
                }
            }
            curve = EmFxSpark_Curves[idx++];
            Key_LerpVec3(curve, p->unk20, p->unk10);
            Vec_Copy(p->pos, fx->pos);
            break;
        case 4:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            curve = EmFxSpark_Curves[idx++];
            Key_LerpVec3(curve, p->unk20, p->unk10);
            break;
        case 5:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            curve = EmFxSpark_Curves[idx++];
            Key_LerpVec3(curve, p->unk20, p->unk10);
            break;
        case 6:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            curve = EmFxSpark_Curves[idx++];
            Key_LerpVec3(curve, p->unk20, p->unk10);
            switch (p->kind) {
            case 0:
                p->rot[2] = Rand_Next(1);
                break;
            case 1:
                p->rot[2] += 0x1000;
                break;
            }
            break;
        case 7:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                idx += step;
                continue;
            }
            k = idx;
            Key_LerpVec3(EmFxSpark_Curves[k], p->unk20, p->unk10);
            idx += 2;
            Key_LerpFloat(EmFxSpark_Curves[(s16) (k + 1)], &a, p->unk10);
            p->alpha = 255.0f * a;
            switch (p->kind) {
            case 0:
                p->rot[2] += 0x1000;
                break;
            }
            break;
        case 8:
            if (p->unk10 <= 0) {
                idx += step;
                continue;
            }
            if (end < p->unk10) {
                EmFxSpark_RestartKind8(fx, p);
            }
            Vec_Copy(p->pos, fx->pos);
            break;
        case 9:
            if (p->kind == 0xFF) {
                continue;
            }
            p->unk30[1] += -8.0f;
            p->pos[0] += p->unk30[0];
            p->pos[1] += p->unk30[1];
            p->pos[2] += p->unk30[2];
            f = AreaGrid2_GetFloorYFiltered(p->pos);
            if (p->pos[1] <= f) {
                p->pos[1] = f;
                if (em->active != 0 && em->type == fx->var2) {
                    Em_PlaySeChecked(em, 0x1E, 0, p->pos, 1, 0);
                }
                EmFxSpark_SpawnAt(p->pos, 1, fx->var2, 0.3f * fx->scale);
                EmFxSpark_SpawnAt(p->pos, 2, fx->var2, 0.3f * fx->scale);
                p->kind = 0xFF;
                continue;
            }
            break;
        }
        if (p->node != NULL) {
            p->node->pos[0] = p->pos[0] + ofs[0];
            p->node->pos[1] = p->pos[1] + ofs[1];
            p->node->pos[2] = p->pos[2] + ofs[2];
            if (p->unk2C != 0) {
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            } else {
                OT_AddByDepth(gDrawOt32, p->node, 0x20, 0);
            }
        }
    }
}

void EmFxSpark_End(StageObj *fx)
{
    EmFxSpark *p = (EmFxSpark *) fx->buf;
    s16 n;
    s16 i;

    fx->state++;
    n = EmFxSpark_Counts[fx->type];
    if (fx->node != NULL) {
        DrawSlot_Free(fx->slot);
    }
    for (i = 0; i < n; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void EmFxSpark_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void EmFxSpark_Draw(DrawSlot *node)
{
    StageObj *fx;
    EmFxSpark *p;
    s16 k;
    Model *mdl;
    ModelMat *tex;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    f32 tm[16];
    u32 color;
    ModelMatSet *mesh;
    u16 flags;
    s32 order;
    u8 mode;
    u8 *curve;
    s32 o;
    u8 alpha;

    flags = 0;
    fx = node->owner;
    p = (EmFxSpark *) fx->buf + node->part;
    k = EmSlot_FindKind(fx->var2);
    if (k != -1 && (mdl = gScene.partModels[k]) != NULL && mdl->active != 0) {
        tex = mdl->mats;
        scale[0] = p->scale * p->unk20[0];
        scale[1] = p->scale * p->unk20[1];
        scale[2] = p->scale * p->unk20[2];
        mode = 1;
        switch (fx->type) {
        case 0:
            if (p->kind == 0) {
                mesh = &mdl->matSets[7];
                rot[0] = (6.2831855f * p->rot[0]) / 65536.0f;
                rot[1] = (6.2831855f * p->rot[1]) / 65536.0f;
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 6;
                Mtx_Translation(tm, 0.0f, 0.046875f * p->unk10, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                alpha = 0xFF;
                mode = 0;
            } else {
                mesh = mdl->matSets;
                flags |= 2;
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 3;
                alpha = 0xFF;
                if (p->unk2C == 1) {
                    alpha = 0xFF;
                } else {
                    alpha = 0x80;
                    scale[0] *= 0.8f;
                    scale[1] *= 0.8f;
                    scale[2] *= 0.8f;
                }
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
            color = (alpha << 24) | 0xFFFFFF;
            goto draw;
        case 1:
            if (p->kind == 0) {
                mesh = &mdl->matSets[3];
                flags |= 2;
            } else {
                mesh = &mdl->matSets[11];
            }
            rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            color = (p->alpha << 24) | 0xFFFFFF;
            goto draw;
        case 2:
            mesh = &mdl->matSets[10];
            rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            color = (p->alpha << 24) | 0xFFFFFF;
            goto draw;
        case 3:
            switch (p->kind) {
            case 0:
                curve = EmFxSpark_Curve0;
                mesh = &mdl->matSets[2];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 3;
                flags |= 2;
                break;
            case 1:
                curve = EmFxSpark_Curve1;
                order = 0;
                mesh = &mdl->matSets[3];
                flags |= 2;
                break;
            case 2:
                curve = EmFxSpark_Curve2;
                mesh = &mdl->matSets[7];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 3;
                Mtx_Translation(tm, 0.0f, 0.0234375f * p->unk10, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                mode = 0;
                break;
            case 3:
                curve = EmFxSpark_Curve3;
                mesh = &mdl->matSets[5];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 3;
                flags |= 2;
                break;
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
            Color_EvalKeys((s32 *)curve, p->unk10, &color);
            if (fx->timer < 20) {
                /* a is set twice, so it stays a value of its own: its mask is made before the
                   red channel and its shift after it (the game's registers at the join) */
                u32 a = (color >> 24) & 0xFF;

                a = (u32) (a * (fx->timer / 20.0f)) & 0xFF;
                color = (color & 0xFF) | ((((color >> 8) & 0xFF) << 8) | ((a << 24) | (((color >> 16) & 0xFF) << 16)));
            }
            goto draw;
        case 4:
            switch (p->kind) {
            case 0:
            case 1:
                mesh = &mdl->matSets[1];
                Mtx_Identity(m);
                Mtx_RotateXYZ(m, (6.2831855f * p->rot[0]) / 65536.0f, (6.2831855f * p->rot[1]) / 65536.0f, 0.0f);
                EmFxSpark_FaceCamera(m, node->pos);
                m[0] *= p->unk20[0];
                m[1] *= p->unk20[0];
                m[2] *= p->unk20[0];
                m[4] *= p->unk20[1];
                m[5] *= p->unk20[1];
                m[6] *= p->unk20[1];
                m[8] *= p->unk20[2];
                m[9] *= p->unk20[2];
                m[10] *= p->unk20[2];
                Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
                flags |= 2;
                mode = 0;
                break;
            case 2:
                mesh = &mdl->matSets[2];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                break;
            case 3:
                mesh = &mdl->matSets[3];
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
                break;
            case 4:
                mesh = &mdl->matSets[4];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                break;
            case 5:
                mesh = &mdl->matSets[8];
                rot[1] = (6.2831855f * p->rot[1]) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, m);
                Mtx_Translation(tm, 0.0f, 0.0234375f * p->unk10, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                mode = 0;
                break;
            }
            curve = EmFxSpark_KindCurves5[p->kind];
            if (curve == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *)curve, p->unk10, &color);
            }
            goto draw;
        case 5:
            switch (p->kind) {
            case 0:
                mesh = &mdl->matSets[5];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                order = 3;
                flags |= 2;
                break;
            case 1:
                mesh = &mdl->matSets[3];
                flags |= 2;
                order = 0;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
                break;
            }
            curve = EmFxSpark_KindCurves1[p->kind];
            if (curve == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *)curve, p->unk10, &color);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
            goto draw;
        case 6:
            mesh = &mdl->matSets[6];
            rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
            flags |= 2;
            color = 0xFFFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            goto draw;
        case 7:
            Gs_SetState(0x6C, 0);
            switch (p->kind) {
            case 0:
                mesh = &mdl->matSets[6];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                o = 3;
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            case 1:
                o = 0;
                mesh = &mdl->matSets[4];
                break;
            case 2:
                mesh = &mdl->matSets[7];
                rot[1] = (6.2831855f * p->rot[1]) / 65536.0f;
                o = 2;
                mode = 0;
                break;
            default:
                return;
            }
            flags |= 2;
            color = (p->alpha << 24) | 0xFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, o, m);
            goto draw;
        case 8:
            Gs_SetState(0x6C, 0);
            switch (p->kind) {
            case 0:
                mesh = mdl->matSets;
                rot[0] = (6.2831855f * p->rot[0]) / 65536.0f;
                rot[1] = (6.2831855f * p->rot[1]) / 65536.0f;
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                o = 6;
                mode = 0;
                Mtx_Translation(tm, 0.0f, (0.1875f * p->unk10) / 10.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            default:
                flags |= 2;
                mesh = &mdl->matSets[6];
                rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
                o = 3;
                Mtx_Translation(tm, 0.0f, 0.25f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            }
            color = 0xFFFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, o, m);
            goto draw;
        case 9:
            mesh = &mdl->matSets[9];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
            color = 0xFFFFFFFF;
            mode = 2;
            goto draw;
        default:
        draw:
            switch (mode) {
            case 1:
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            case 2:
                Mtx_Concat3x3(m, gCamYawMtx);
                break;
            }
            if (p->unk2C != 0) {
                Effect_DrawModel(mesh, m, color, flags, tex);
            } else {
                Gs_SetState(0x67, color);
                Effect_DrawModelAdd(mesh, m, tex);
            }
            Gs_SetState(0x6C, 1);
            break;
        }
    }
}

void EmFxSpark_BonePos(f32 *out, Em *em, f32 *ofs, s32 bone)
{
    f32 b[3];
    f32 m[16];

    Mtx_Copy(m, Em_Bone(em, bone));
    Vec_Rotate(b, ofs, m);
    out[0] = m[12] + b[0];
    out[1] = m[13] + b[1];
    out[2] = m[14] + b[2];
}

void EmFxSpark_PickBones(StageObj *fx, EmFxSpark *p)
{
    s16 pick[4];
    s16 used[4];
    s16 i;
    s16 j;
    s16 n;

    p++;
    for (i = 0; i < 4; i++, p++) {
        pick[i] = Rand_Next(1) % (10 - i);
        for (j = 0; j < i; j++) {
            used[j] = 0;
        }
        for (;;) {
            n = 0;
            for (j = 0; j < i; j++) {
                if (used[j] == 0 && pick[j] <= pick[i]) {
                    used[j] = 1;
                    n++;
                }
            }
            if (n <= 0) {
                break;
            }
            pick[i] += n;
        }
        p->bone = pick[i];
        p->rot[2] = Rand_Next(1);
        p->scale = EmFxSpark_Bones[p->bone].scale * (0.9f + 2.0000001e-4f * (Rand_Next(1) & 0x3FF));
        EmFxSpark_BonePos(p->pos, fx->owner, EmFxSpark_Bones[p->bone].ofs, EmFxSpark_Bones[p->bone].bone);
    }
}

void EmFxSpark_RestartKind3(StageObj *fx, EmFxSpark *p)
{
    s16 temp_a1;

    temp_a1 = p->kind;
    switch (temp_a1) {
    case 0:
        p->unk10 = 1;
        p->rot[2] = Rand_Next(1);
        break;
    case 1:
        p->unk10 = 1;
        break;
    case 2:
        p->unk10 = 1;
        p->rot[2] = Rand_Next(1);
        break;
    case 3:
        p->unk10 = -1;
        p->rot[2] = Rand_Next(1);
        break;
    }
}

void EmFxSpark_RestartKind8(StageObj *fx, EmFxSpark *p)
{
    p->unk10 = 1;
    switch (p->kind) {
    case 0:
        p->rot[0] = Rand_Next(1);
        p->rot[1] = Rand_Next(1);
        p->rot[2] = Rand_Next(1);
        break;
    default:
        p->rot[2] = Rand_Next(1);
        p->scale = (f32) (1.2f + (0.0006f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF)));
        break;
    }
}

void EmFxSpark_FaceCamera(f32 *dst, void *pos)
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

void EmFxSpark_SpawnOnEm(Em *em, u8 k)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = 4;
            obj->update = EmFxSpark_Update;
            obj->type = k;
            obj->owner = em;
            obj->var2 = em->type;
            obj->node = NULL;
            obj->scale = 1.0f;
        }
    }
}

void EmFxSpark_SpawnAt(void *src, u8 k, u8 c, f32 f)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 4;
        obj->update = EmFxSpark_Update;
        obj->type = k;
        obj->owner = NULL;
        obj->var2 = c;
        obj->node = NULL;
        obj->scale = f;
        Vec_Copy(obj->pos, (f32 *) src);
    }
}

void EmFxSpark_SpawnTimed(Em *em, u8 k, s32 n, f32 f)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = 4;
            obj->update = EmFxSpark_Update;
            obj->type = k;
            obj->owner = em;
            obj->var2 = em->type;
            obj->node = NULL;
            obj->scale = f;
            if (em->motionSpeed < 0.001f) {
                n = 100;
            } else {
                n = n / (2.0f * em->motionSpeed);
                if (n > 200) {
                    n = 100;
                }
            }
            obj->angle = n;
        }
    }
}

void StageObj_K05_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        StageObj_K05_Setup(fx);
        break;
    case 1:
        StageObj_K05_Run(fx);
        break;
    case 2:
        StageObj_K05_End(fx);
        break;
    case 3:
        StageObj_K05_Free(fx);
        break;
    }
}

void StageObj_K05_Setup(StageObj *fx)
{
    EmFxMtxBuf *buf = (EmFxMtxBuf *) fx->buf;
    Em *em = fx->owner;
    s32 n;
    s16 i;
    s16 j;
    f32 m[16];
    f32 pos[3];

    fx->state++;
    fx->var0 = 0;
    fx->visible = 1;
    fx->draw = 0;
    n = D_008CDB28[fx->type];
    for (i = 0; i < n; i++) {
        if (fx->keep == 1) {
            buf->slot[i] = DrawSlotB_Alloc();
        } else {
            buf->slot[i] = DrawSlot_Alloc();
        }
        if (buf->slot[i] != -1) {
            if (fx->keep == 1) {
                buf->node[i] = DrawSlotB_Get(buf->slot[i]);
            } else {
                buf->node[i] = DrawSlot_Get(buf->slot[i]);
            }
            buf->node[i]->owner = fx;
            buf->node[i]->part = i;
            buf->node[i]->draw = StageObj_K05_Draw;
        } else {
            buf->node[i] = NULL;
        }
        StageObj_K05_GetOffset(pos, fx, em, i);
        StageObj_K05_GetMatrix(m, fx, em, i);
        Vec_RotateSelf(pos, m);
        m[12] += pos[0];
        m[13] += pos[1];
        m[14] += pos[2];
        for (j = 0; j < 6; j++) {
            Mtx_Copy(buf->m[j][i], m);
        }
    }
    StageObj_K05_Run(fx);
}

void StageObj_K05_Run(StageObj *fx)
{
    Em *em = fx->owner;
    EmFxMtxBuf *buf = (EmFxMtxBuf *) fx->buf;
    s32 n;
    s16 i;
    s16 j;
    f32 m[16];
    f32 pos[3];

    if (em->active == 0) {
        fx->visible = 0;
        fx->state++;
        return;
    }
    if (fx->timer != -1) {
        if (--fx->timer <= 0 || (em->motionId[0] != (u16) fx->angle && em->motionId[0] != fx->angle2)) {
            fx->visible = 0;
            fx->state++;
            return;
        }
    }
    n = D_008CDB28[fx->type];
    for (i = 0; i < n; i++) {
        for (j = 5; j > 0; j--) {
            Mtx_Copy(buf->m[j][i], buf->m[j - 1][i]);
        }
        StageObj_K05_GetOffset(pos, fx, em, i);
        StageObj_K05_GetMatrix(m, fx, em, i);
        Vec_RotateSelf(pos, m);
        m[12] += pos[0];
        m[13] += pos[1];
        m[14] += pos[2];
        Mtx_Copy(buf->m[0][i], m);
        if (fx->timer == -1) {
            if (!(em->statusIcons & 0x8000)) {
                if (em->motionId[0] != 0x3FB || Motion_ReachedFrame(em, 0, 22.0f)) {
                    continue;
                }
            }
            if (em->motionId[0] == 0x3FA && !Motion_ReachedFrame(em, 0, 32.0f)) {
                continue;
            }
            if ((u32) (em->motionId[0] - 0x3C) <= 1 || em->stateGroup == 2 || em->stateGroup == 6) {
                continue;
            }
        }
        if (((Em *)fx->owner)->visible != 0 && (Obj_InCurrentArea(fx->owner) & 0xFF) && buf->node[i] != NULL) {
            Mtx_GetTrans(buf->node[i]->pos, buf->m[0][i]);
            OT_AddByDepth(gDrawOtSorted, buf->node[i], 0x40, 0);
        }
    }
}


void StageObj_K05_End(StageObj *fx)
{
    EmFxMtxBuf *x = (EmFxMtxBuf *) fx->buf;
    s16 n;
    s16 i;

    fx->state++;
    n = D_008CDB28[fx->type];
    for (i = 0; i < n; i++) {
        if (x->node[i] != NULL) {
            if (fx->keep == 1) {
                DrawSlotB_Free(x->slot[i]);
            } else {
                DrawSlot_Free(x->slot[i]);
            }
        }
    }
}

void StageObj_K05_Free(StageObj *fx)
{
    StageObj_Free(fx);
}

/* sub_main's five common model set pointers (gCommonModels) as a named object: its load moves
   above the flags = 0 stack store (an absolute address may alias the stack store) */

extern u8 gEffectColors;
extern u8 D_0041CEC1;
extern u8 D_0041CEC2;
void StageObj_K05_Draw(DrawSlot *node)
{
    u8 a;
    s32 n;
    s32 j;
    s32 k;
    StageObj *fx;
    Model *mdl;
    Em *em;
    ModelMat *tex;
    ModelMatSet *mesh;
    u8 r;
    u8 g;
    u8 b;
    u8 *bones;
    s16 t;
    f32 (*p)[2][16];
    EmFxMtxBuf *buf;
    f32 len;
    s16 flags;
    f32 f;
    f32 m[16];
    f32 m0[16];
    f32 m1[16];
    f32 m2[16];
    f32 pos[3];

    fx = node->owner;
    mdl = gCommonModels[4];
    flags = 0;
    buf = (EmFxMtxBuf *) fx->buf;
    em = fx->owner;
    if (mdl == NULL || em->visible == 0) {
        return;
    } else if (mdl->active != 0) {
        bones = (u8 *) mdl->bones;
        n = *(s16 *) (bones + 0xC2);
        StageObj_K05_GetOffset(pos, fx, em, (s16) node->part);
        StageObj_K05_GetMatrix(m, fx, em, (s16) node->part);
        Vec_RotateSelf(pos, m);
        m[12] += pos[0];
        m[13] += pos[1];
        m[14] += pos[2];
        Mtx_Copy((f32 *) bones, m);
        Mtx_Copy((f32 *) (bones + 0x190), m);
        bones += 0x320;
        n -= 2;
        p = (void *) buf->m[0][node->part];
        len = Vec_Length((f32 *) ((u8 *) m + 0x20));
        for (j = 0; j < 4; j++) {
            Mtx_Copy(m0, p[j][0]);
            Mtx_Copy(m1, p[j + 1][0]);
            Mtx_Copy(m2, p[j + 2][0]);
            for (k = 0; k < 6; k++) {
                Mtx_Blend(m, m0, m1, StageObj_K05_Weights2[k][0], StageObj_K05_Weights2[k][1]);
                m[8] *= len;
                m[9] *= len;
                m[10] *= len;
                Mtx_SetTrans(m,
                    m1[12] * StageObj_K05_Weights3[k][1] + (m0[12] + m1[12]) / 2.0f * StageObj_K05_Weights3[k][0] + (m1[12] + m2[12]) / 2.0f * StageObj_K05_Weights3[k][2],
                    m1[13] * StageObj_K05_Weights3[k][1] + (m0[13] + m1[13]) / 2.0f * StageObj_K05_Weights3[k][0] + (m1[13] + m2[13]) / 2.0f * StageObj_K05_Weights3[k][2],
                    m1[14] * StageObj_K05_Weights3[k][1] + (m0[14] + m1[14]) / 2.0f * StageObj_K05_Weights3[k][0] + (m1[14] + m2[14]) / 2.0f * StageObj_K05_Weights3[k][2]);
                Mtx_Copy((f32 *) bones, m);
                n--;
                bones += 0x190;
                if (n <= 0) {
                    goto done;
                }
            }
        }
    done:
        Bone_StoreMatrices((struct EmBone *) ((u8 *) mdl->bones));
        Gfx_SetTexFilter(1);
        mesh = &mdl->matSets[D_008CDB18[fx->type]];
        tex = mdl->mats;
        switch (fx->type) {
        default:
            r = gEffectColors;
            g = D_0041CEC1;
            b = D_0041CEC2;
            a = 0xFF;
            break;
        case 1:
        case 2:
        case 4:
            r = 0xFF;
            g = 0xFF;
            b = 0xFF;
            a = 0xFF;
            flags |= 2;
            break;
        case 3:
            r = 0xFF;
            g = 0x33;
            a = 0xFF;
            b = 0x33;
            flags |= 2;
            break;
        case 5:
            t = *(s16 *) &em->scriptCallEntry;
            flags |= 2;
            if (t < 60) {
                f = 0.0f;
            } else {
                f = (t - 60) / 60.0f;
            }
            if (f > 1.0f) {
                f = 1.0f;
            }
            r = 0xFF;
            g = 0xFF - (u8) (204.0f * f);
            b = g;
            if (t < 20) {
                f = 0.0f;
            } else {
                f = (t - 20) / 20.0f;
            }
            if (f > 1.0f) {
                f = 1.0f;
            }
            a = 255.0f * f;
            break;
        case 6:
            r = 0xFF;
            g = 0x33;
            a = 0xFF;
            b = 0x33;
            flags |= 2;
            break;
        }
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x67, (a << 24) | (r << 16) | (g << 8) | b);
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(tex, mesh);
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags((u16) flags);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K05_GetMatrix(f32 *m, StageObj *fx, Em *em, s16 alt)
{
    s32 idx;
    f32 one;
    f32 s;

    idx = Player_DualSwordIndex(em);
    if (idx != -1) {
        if (fx->type == 1 || (fx->type == 3 && alt == 0)) {
            one = 1.0f;
            s = StageObj_K05_ScalesA[idx];
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0xE));
        } else {
            one = 1.0f;
            s = StageObj_K05_ScalesB[idx];
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0x12));
            Mtx_TurnZ(m, 3.1415927f);
        }
        m[0] *= one;
        m[1] *= one;
        m[2] *= one;
        m[4] *= one;
        m[5] *= one;
        m[6] *= one;
        m[8] *= s;
        m[9] *= s;
        m[10] *= s;
    } else {
        switch (fx->type) {
        case 4:
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0x12));
            Mtx_TurnY(m, 3.1415927f);
            m[8] *= 1.5f;
            m[9] *= 1.5f;
            m[10] *= 1.5f;
            break;
        case 5:
        case 6:
            Mtx_Copy(m, (f32 *) Em_GetBone(em, 0xE));
            m[8] *= 2.1f;
            m[9] *= 2.1f;
            m[10] *= 2.1f;
            break;
        default:
            if (em->unk417 != 0) {
                Mtx_Copy(m, (f32 *) Em_GetBone(em, 0xE));
            } else {
                Mtx_Copy(m, (f32 *) Em_GetBone(em, 0x12));
            }
            break;
        }
    }
}

void StageObj_K05_GetOffset(f32 *out, StageObj *fx, void *obj, s16 alt)
{
    s32 idx;

    idx = Player_DualSwordIndex(obj);
    if (idx != -1) {
        if (fx->type == 1 || (fx->type == 3 && alt == 0)) {
            out[0] = StageObj_K05_OffsetsB[idx].x;
            out[1] = D_008CDB44[idx].x;
            out[2] = D_008CDB48[idx].x;
        } else {
            out[0] = StageObj_K05_OffsetsC[idx].x;
            out[1] = D_008CDCC4[idx].x;
            out[2] = D_008CDCC8[idx].x;
        }
    } else {
        out[0] = StageObj_K05_Offsets[fx->type].x;
        out[1] = D_008CDAC4[fx->type].x;
        out[2] = D_008CDAC8[fx->type].x;
    }
}

StageObj *StageObj_K05_Create(void *owner, s32 arg1, s32 kind)
{
    ObjHead *o = owner;
    s16 always = 0;
    StageObj *obj;

    switch ((s16) kind) {
    case 2:
        if (((Em *) owner)->statusIcons & 0x8000) { /* a monster's kind */
            return NULL;
        }
        break;
    case 3:
        always = 1;
        break;
    case 5:
        if (*(s16 *) &((Em *) owner)->scriptCallEntry == 100) { /* a monster's kind */
            kind = 6;
        }
        break;
    }
    if (always == 0 && !(Obj_InCurrentArea(owner) & 0xFF)) {
        return NULL;
    }
    obj = StageObj_Alloc(2);
    if (obj == NULL) {
        return NULL;
    }
    obj->kind = 5;
    obj->update = StageObj_K05_Update;
    obj->draw = NULL;
    obj->owner = owner;
    obj->ownerIndex = o->index;
    obj->timer = arg1;
    obj->angle = o->motionId[0];
    obj->type = kind;
    if (always) {
        obj->keep = 1;
    }
    obj->angle2 = 0;
    return obj;
}

void StageObj_K05_CreateEx(Em *em, s32 a1, s32 a2, s16 v)
{
    StageObj *p = StageObj_K05_Create(em, a1, a2);

    if (p != NULL) {
        p->angle2 = v;
    }
}

void EmFxTri_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        EmFxTri_Setup(fx);
        break;
    case 1:
        EmFxTri_Move(fx);
        break;
    case 2:
        EmFxTri_End(fx);
        break;
    case 3:
        EmFxTri_Delete(fx);
        break;
    }
}

void EmFxTri_Setup(StageObj *fx)
{
    EmFxTri *b = (EmFxTri *) fx->buf;
    Em *em = fx->owner;
    u8 i;
    f32 v[3];

    fx->state++;
    fx->var0 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    fx->angle = 0;
    switch (fx->type) {
    case 2:
        for (i = 0; i < 3; i++) {
            b->slot[i] = DrawSlot_Alloc();
            if (b->slot[i] != -1) {
                b->node[i] = DrawSlot_Get(b->slot[i]);
                b->node[i]->owner = fx;
                b->node[i]->part = i;
                b->node[i]->draw = EmFxTri_Draw;
            } else {
                b->node[i] = NULL;
            }
        }
        return;
    case 3:
        fx->var2 = EmFxTri_AreaType(em);
        fx->angle2 = 0;
        Mtx_Copy(b->m, Em_Bone(em, 0xB));
        Vec_Rotate(v, EmFxTri_Offsets3[fx->var2], b->m);
        fx->pos[0] = b->m[12] + v[0];
        fx->pos[1] = b->m[13] + v[1];
        fx->pos[2] = b->m[14] + v[2];
        break;
    case 4:
        fx->var2 = 5;
        break;
    case 5:
        fx->timer = 70;
        fx->scale = 0.7f;
        fx->angle2 = 0;
        for (i = 0; i < 3; i++) {
            b->slot[i] = DrawSlot_Alloc();
            if (b->slot[i] != -1) {
                b->node[i] = DrawSlot_Get(b->slot[i]);
                b->node[i]->owner = fx;
                b->node[i]->part = i;
                b->node[i]->draw = EmFxTri_Draw;
            } else {
                b->node[i] = NULL;
                continue;
            }
            v[0] = EmFxTri_Vecs5[i][0];
            v[1] = EmFxTri_Vecs5[i][1];
            v[2] = EmFxTri_Vecs5[i][2];
            Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
            Vec_Copy(b->node[i]->pos, (f32 *) &em->posX);
            b->node[i]->pos[0] += v[0];
            b->node[i]->pos[1] += v[1];
            b->node[i]->pos[2] += v[2];
        }
        return;
    }
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = EmFxTri_Draw;
    } else {
        StageObj_Free((struct StageObj *) fx);
    }
}

void EmFxTri_Move(StageObj *fx)
{
    EmFxTri *t;
    u8 i;
    Em *em;
    f32 *m;
    f32 v[3];

    em = fx->owner;
    t = (EmFxTri *) fx->buf;
    if (em->active == 0 || em->taskPhase == 3) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    switch (fx->type) {
    case 0:
        switch (fx->var0) {
        case 0:
            if (em->rage != 0 && --fx->timer <= 0) {
                Em_PlaySeChecked(em, 0x19, 0, &em->posX, 1, 0);
                if (!(Rand_Next(1) & 3)) {
                    fx->var0 = 1;
                    fx->angle = Rand_Next(1);
                    fx->var1 = Rand_Next(1);
                    fx->var2 = Rand_Next(1);
                } else {
                    fx->var0 = 2;
                }
                fx->timer = 0;
            }
            break;
        case 1:
            if (++fx->timer >= 10) {
                fx->var0 = 0;
                fx->timer = (Rand_Next(1) & 0xF) + 4;
                break;
            }
            Em_BonePos(em, 2, fx->pos);
            Vec_Copy(fx->node->pos, (f32 *) fx->pos);
            OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
            break;
        case 2:
            if (++fx->timer >= 12) {
                fx->var0 = 0;
                fx->timer = (Rand_Next(1) & 0xF) + 4;
                break;
            }
            Em_BonePos(em, 0x17, fx->pos);
            Vec_Copy(fx->node->pos, (f32 *) fx->pos);
            OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
            break;
        }
        break;
    case 1:
        fx->timer++;
        fx->scale = 1.0f + 0.1f * Math_Sin((6.2831855f * (fx->timer << 10)) / 65536.0f);
        fx->scale *= em->scaleX;
        Em_BonePos(em, 0xD, fx->pos);
        Vec_Copy(fx->node->pos, (f32 *) fx->pos);
        if (em->rage != 0) {
            OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
        }
        break;
    case 2:
        fx->timer++;
        fx->var0 = func_0083BA70(em);
        switch (fx->var0) {
        case 0:
            if (--fx->angle <= 0) {
                fx->angle = 0;
            }
            break;
        case 1:
            if (++fx->angle >= 15) {
                fx->angle = 15;
            }
            break;
        }
        fx->var2 = 0;
        if (em->stateGroup == 5) {
            if (++fx->var1 < 4) {
                fx->var2 = 1;
            } else {
                fx->var1 = 4;
            }
            fx->scale -= 0.02f;
            if (fx->scale < 0.5f) {
                fx->scale = 0.5f;
            }
        } else {
            fx->scale = 1.0f;
        }
        if (em->visible != 0) {
            Em_BonePos(em, 2, fx->pos);
            for (i = 0; i < 3; i++) {
                if (t->node[i] != NULL && (i != 2 || fx->var2 != 0)) {
                    Vec_Copy(t->node[i]->pos, (f32 *) fx->pos);
                    OT_AddByDepth(gDrawOtSorted, t->node[i], 0x40, 0);
                }
            }
        }
        break;
    case 3:
        fx->timer++;
        switch (fx->var0) {
        case 0:
            if (em->motionId[0] == 0x3F7 || (em->motionId[0] == 0x3F6 && EmMotion_ReachedFrame(em, 0, 32.0f) == 0)) {
                Mtx_Copy(t->m, Em_Bone(em, 0xB));
                Vec_Rotate(v, EmFxTri_Offsets3[fx->var2], t->m);
                fx->pos[0] = t->m[12] + v[0];
                fx->pos[1] = t->m[13] + v[1];
                fx->pos[2] = t->m[14] + v[2];
            } else {
                fx->var0++;
                fx->scale = em->floorY;
                fx->angle = Rand_Next(1);
                fx->timer = 0;
            }
            break;
        case 1:
        case 2:
            fx->angle2 += 0x1000;
            if (fx->var0 < 2) {
                v[1] = 25.0f / fx->var0 - 3.5f * fx->timer;
            } else {
                v[1] = 60.0f / fx->var0 - 3.5f * fx->timer;
            }
            v[0] = 0.0f;
            v[2] = 7.5f / fx->var0;
            Vec_RotateY(v, (6.2831855f * fx->angle) / 65536.0f);
            fx->pos[0] += v[0];
            fx->pos[1] += v[1];
            fx->pos[2] += v[2];
            if (fx->pos[1] <= fx->scale) {
                fx->pos[1] = fx->scale;
                fx->var0++;
                fx->timer = 0;
            }
            break;
        case 3:
            fx->state++;
            fx->visible = 0;
            return;
        }
        Vec_Copy(fx->node->pos, (f32 *) fx->pos);
        switch (fx->var2) {
        case 3:
            OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
            break;
        default:
            OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
            break;
        }
        break;
    case 4:
        fx->timer++;
        m = Em_Bone(em, 0xB);
        Vec_Rotate(v, EmFxTri_Offsets3[fx->var2], m);
        fx->pos[0] = m[12] + v[0];
        fx->pos[1] = m[13] + v[1];
        fx->pos[2] = m[14] + v[2];
        Vec_Copy(fx->node->pos, (f32 *) fx->pos);
        if (em->alpha < 1.0f) {
            OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
        } else {
            OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
        }
        break;
    case 5:
        switch (fx->var0) {
        case 0:
            fx->angle2 += 0x10;
            break;
        case 1:
            fx->angle2 -= 0x10;
            break;
        }
        if (fx->timer-- <= 0) {
            fx->timer = 70;
            fx->var0 = (fx->var0 + 1) & 1;
        }
        for (i = 0; i < 3; i++) {
            if (t->node[i] != NULL) {
                t->ofs[0] = EmFxTri_Vecs5[i][0];
                t->ofs[1] = EmFxTri_Vecs5[i][1];
                t->ofs[2] = EmFxTri_Vecs5[i][2];
                Vec_RotateY(t->ofs, (6.2831855f * em->rotY) / 65536.0f);
                Vec_Copy(t->node[i]->pos, (f32 *) &em->posX);
                t->node[i]->pos[0] += t->ofs[0];
                t->node[i]->pos[1] += t->ofs[1];
                t->node[i]->pos[2] += t->ofs[2];
                OT_AddByDepth(gDrawOt32, t->node[i], 0x20, 0);
            }
        }
        break;
    }
}


void EmFxTri_End(StageObj *fx)
{
    EmFxTri *x = (EmFxTri *) fx->buf;
    u8 i;

    fx->state++;
    switch (fx->type) {
    case 2:
        for (i = 0; i < 3; i++) {
            if (x->node[i] != NULL) {
                DrawSlot_Free(x->slot[i]);
            }
        }
        break;
    case 5:
        for (i = 0; i < 3; i++) {
            if (x->node[i] != NULL) {
                DrawSlot_Free(x->slot[i]);
            }
        }
        break;
    default:
        DrawSlot_Free(fx->slot);
        break;
    }
}

void EmFxTri_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

void EmFxTri_Draw(DrawSlot *node)
{
    StageObj *fx;
    Model *mdl;
    ModelMatSet *mesh;
    ModelMat *tex;
    Em *em;
    u16 flags;
    s16 k;
    s16 t;
    s32 div;
    u16 rot;
    u8 r;
    u8 g;
    u8 b;
    u8 al;
    u8 alpha;
    f32 a;
    u32 color;
    f32 scale[3];
    f32 m[16];
    f32 tm[16];

    flags = 0;
    fx = node->owner;
    em = fx->owner;
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return;
    }
    switch (fx->type) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
        k = EmSlot_FindKind(em->type);
        if (k == -1) {
            return;
        }
        mdl = gScene.partModels[k];
        break;
    case 2:
        mdl = gCommonModels[0];
        break;
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    tex = mdl->mats;
    switch (fx->type) {
    case 0:
        Mtx_Identity(m);
        switch (fx->var0) {
        case 1:
            div = 10;
            Mtx_CopyRot(m, Em_Bone(em, 2));
            Mtx_RotateXYZ(m, (6.2831855f * fx->angle) / 65536.0f, (6.2831855f * (fx->var1 << 8)) / 65536.0f, (6.2831855f * (fx->var2 << 8)) / 65536.0f);
            break;
        case 2:
            div = 12;
            mesh = &mdl->matSets[8];
            Mtx_Translation(tm, 0.0f, 0.1875f * (fx->timer / 12.0f), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            Mtx_Copy(m, Em_Bone(em, 2));
            Gs_SetState(0x1A, (s32) m);
            if (mesh != NULL) {
                if (mesh->id != -1) {
                    Gs_SetLightList(tex, mesh);
                    Draw_ApplyStates(mesh->flags);
                    Model_Draw(mesh->id, 0);
                }
                Draw_SetDefaultState();
            }
            Mtx_Identity(m);
            Mtx_CopyRot(m, Em_Bone(em, 0x17));
            break;
        default:
            return;
        }
        Mtx_Translation(tm, 0.0f, 0.1875f * (fx->timer / (f32) div), 0.0f);
        Gs_SetState(0x19, (s32) tm);
        mesh = &mdl->matSets[fx->var0] - 1;
        break;
    case 1:
        flags |= 2;
        Mtx_Scale(m, fx->scale, fx->scale, fx->scale);
        Mtx_Concat3x3(m, gCamMatrix);
        mesh = &mdl->matSets[2];
        color = 0x80FFFFFF;
        break;
    case 2:
        flags |= 2;
        switch (node->part) {
        case 0:
            t = fx->angle;
            if (t == 0) {
                a = 1.0f;
            } else if (t == 15) {
                a = 0.0f;
            } else {
                a = 1.0f - t / 15.0f;
            }
            mesh = &mdl->matSets[64];
            scale[0] = scale[1] = scale[2] = EmFxTri_Blink0[fx->timer & 1];
            al = b = 0xFF;
            g = r = 0xA0;
            rot = 0;
            break;
        case 1:
            t = fx->angle;
            if (t == 0) {
                a = 0.0f;
            } else if (t == 15) {
                a = 1.0f;
            } else {
                a = t / 15.0f;
            }
            mesh = &mdl->matSets[81];
            al = b = g = r = 0xFF;
            scale[0] = scale[1] = scale[2] = EmFxTri_Blink1[fx->timer & 1];
            rot = fx->timer << 11;
            break;
        case 2:
            a = 1.0f;
            mesh = &mdl->matSets[81];
            Key_LerpVec2(EmFxTri_ScaleKeys2, scale, fx->var1);
            Color_EvalKeys((s32 *)EmFxTri_ColorKeys2, fx->var1, &color);
            rot = 0;
            g = color >> 8;
            r = color >> 16;
            b = color;
            al = color >> 24;
            break;
        default:
            return;
        }
        scale[0] *= fx->scale * em->scaleX;
        scale[1] *= fx->scale * em->scaleY;
        scale[2] *= fx->scale * em->scaleZ;
        a *= em->alpha;
        a *= -1.0f + 2.0f * fx->scale;
        if (a < 0.0f) {
            a = 0.0f;
        }
        al = a * al;
        Mtx_Scale(m, scale[0], scale[1], scale[2]);
        Mtx_RotateZ(m, (6.2831855f * rot) / 65536.0f);
        Mtx_Concat3x3(m, gCamMatrix);
        color = b | ((g << 8) | ((al << 24) | (r << 16)));
        Gs_SetState(0x6C, 0);
        break;
    case 3:
        if (fx->var2 == 0) {
            return;
        }
        Mtx_Identity(m);
        Mtx_RotateX(m, (6.2831855f * fx->angle2) / 65536.0f);
        Mtx_RotateY(m, (6.2831855f * fx->angle) / 65536.0f);
        mesh = &mdl->matSets[fx->var2] - 1;
        color = ((u8) (255.0f * em->alpha) << 24) | 0xFFFFFF;
        break;
    case 4:
        Mtx_Copy(m, Em_Bone(em, 0xB));
        mesh = &mdl->matSets[fx->var2] - 1;
        color = ((u8) (255.0f * em->alpha) << 24) | 0xFFFFFF;
        break;
    case 5:
        flags |= 2;
        Mtx_Scale(m, fx->scale, fx->scale, fx->scale);
        Mtx_RotateZ(m, (6.2831855f * fx->angle2) / 65536.0f);
        mesh = mdl->matSets;
        color = 0xFFFFFFFF;
        break;
    }
    Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
    switch (fx->type) {
    case 0:
    case 5:
    default:
        Effect_DrawModelAdd(mesh, m, tex);
        break;
    case 1:
    case 2:
        Effect_DrawModel(mesh, m, color, flags, tex);
        break;
    case 3:
        switch (fx->var2) {
        case 3:
            Effect_DrawModel(mesh, m, color, flags, tex);
            break;
        default:
            Effect_DrawModelAdd(mesh, m, tex);
            break;
        }
        break;
    case 4:
        if (em->alpha < 1.0f) {
            Effect_DrawModel(mesh, m, color, flags, tex);
        } else {
            Effect_DrawModelAdd(mesh, m, tex);
        }
        break;
    }
    Gs_SetState(0x6C, 1);
}

u8 EmFxTri_AreaType(Em *em)
{
    s32 dayId;

    dayId = Area_DayId(em->area) & 0xFFFF;
    switch (dayId) {
    case AREA_DESERT_2:
    case AREA_DESERT_5:
        return 1U;
    case AREA_SNOWY_MOUNTAINS_2:
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_JUNGLE_8:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
        return 3U;
    case AREA_SNOWY_MOUNTAINS_4:
    case AREA_SNOWY_MOUNTAINS_6:
        return 0U;
    case AREA_FOREST_HILLS_6:
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_10:
    case AREA_FOREST_HILLS_8:
    case AREA_FOREST_HILLS_9:
    case AREA_FOREST_HILLS_4:
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_11:
    case AREA_JUNGLE_1:
    case AREA_JUNGLE_7:
    case AREA_JUNGLE_9:
    case AREA_SWAMP_5:
    case AREA_SWAMP_6:
    case AREA_SWAMP_7:
        return 2U;
    case AREA_VOLCANO_1:
    case AREA_VOLCANO_6:
    case AREA_VOLCANO_7:
    case AREA_VOLCANO_8:
        return 4U;
    default:
        return 0U;
    }
}

void EmFxTri_Spawn(Em *em, s16 kind)
{
    s16 pool;
    StageObj *obj;

    switch (kind) {
    case 2:
    case 3:
    case 5:
        pool = 1;
        break;
    case 4:
        pool = 0;
        StageObj_K0E_CreateOnEm(em, 0xC);
        break;
    default:
        pool = 0;
        break;
    }
    obj = StageObj_Alloc(pool);
    if (obj != NULL) {
        obj->kind = 7;
        obj->update = EmFxTri_Update;
        obj->owner = em;
        obj->type = kind;
    }
}

void StageObj_K09_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        StageObj_K09_Setup(fx);
        break;
    case 1:
        StageObj_K09_Run(fx);
        break;
    case 2:
        StageObj_K09_End(fx);
        break;
    case 3:
        StageObj_K09_Free(fx);
        break;
    }
}

void StageObj_K09_Setup(StageObj *fx)
{
    Em *m = fx->owner;

    fx->state++;
    fx->var0 = 0;
    fx->var1 = 0;
    fx->var2 = 0xFF;
    fx->type = 0;
    fx->visible = 1;
    fx->draw = 0;
    if (m->type == 0x22) {
        fx->keep = 0;
    } else {
        fx->keep = 1;
    }
    if (fx->keep == 1) {
        fx->slot = DrawSlotB_Alloc();
    } else {
        fx->slot = DrawSlot_Alloc();
    }
    if (fx->slot != -1) {
        if (fx->keep == 1) {
            fx->node = DrawSlotB_Get(fx->slot);
        } else {
            fx->node = DrawSlot_Get(fx->slot);
        }
        fx->node->owner = fx;
        fx->node->draw = StageObj_K09_Draw;
    } else {
        StageObj_Free((struct StageObj *) fx);
    }
}

void StageObj_K09_FollowBone(Em *em, StageObj *fx, s16 mode)
{
    EmBone *bone;
    f32 pos[3];

    if (mode == 0) {
        bone = em->model->bones;
        bone = &bone[StageObj_K09_BoneForKind(em) & 0xFF];
        Mtx_GetTrans(fx->pos, bone->world);
        fx->angle = (Mtx_Heading((f32 *) bone) & 0xFFFF) + 0x4000;
        if (gScene.area == fx->angle2) {
            Em_BonePos(em, StageObj_K09_BoneForKind(em) & 0xFF, pos);
            pos[1] += 100.0f;
            fx->pos[1] = 30.0f + AreaGrid2_GetFloorY(pos);
        }
    } else {
        Vec_Copy(fx->pos, &em->posX);
        fx->angle = em->rotY;
    }
}

void StageObj_K09_CreateMine(StageObj *fx)
{
    s32 mine;

    if ((s32) fx->var0 <= 0) {
        mine = EmHolder_CreateMine((struct EmHolder *)fx, 1, fx->pos);
        if (mine == -1) {
            fx->var2 = 0xFF;
            fx->var0 = 2;
        } else {
            fx->var2 = (u8) mine;
            fx->var0 = 1;
        }
    }
    fx->type = 1;
}

void StageObj_K09_Place(StageObj *fx)
{
    Em *em;

    em = fx->owner;
    StageObj_K09_CreateMine(fx);
    StageObj_K09_FollowBone(em, fx, 0);
}

void StageObj_K09_Run(StageObj *fx)
{
    Em *em = fx->owner;
    u8 *q;

    q = (u8 *)QuestEm_FindForEm(em);
    if (q != NULL && fx->timer > (s8) q[4] && *(s16 *) (q + 0x2E) == 2) {
        fx->state++;
        if (fx->var2 != 0xFF) {
            MinePoint_Remove(fx->var2);
        }
        return;
    }
    if (em->active == 0) {
        fx->state++;
        if (fx->var2 != 0xFF) {
            MinePoint_Remove(fx->var2);
        }
        return;
    }
    switch (fx->var0) {
    case 0:
        fx->angle2 = em->area;
        switch (em->type) {
        case 0x01:
        case 0x2A:
        case 0x25:
        case 0x0B:
        case 0x29:
        case 0x31:
        case 0x0E:
        case 0x2B:
        case 0x1A:
        case 0x2C:
            if (em->stateGroup == 4 && em->state == 0xF) {
                StageObj_K09_Place(fx);
            }
            break;
        case 0x11:
        case 0x2F:
        case 0x16:
            if (em->stateGroup == 4 && em->state == 0x11) {
                StageObj_K09_Place(fx);
            }
            break;
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            switch (fx->var1) {
            case 0:
                if ((em->stateGroup == 4 && em->state == 6) || (em->fxFlags & 1)) {
                    fx->var1 += 2;
                    StageObj_K09_FollowBone(em, fx, 1);
                }
                break;
            default:
                fx->var1 += 2;
                if (fx->var1 > 0x54) {
                    StageObj_K09_CreateMine(fx);
                }
                break;
            }
            break;
        case 0x3B:
            switch (fx->var1) {
            case 0:
                if ((em->stateGroup == 4 && em->state == 4) || (em->fxFlags & 1)) {
                    fx->var1 += 2;
                    StageObj_K09_FollowBone(em, fx, 1);
                }
                break;
            default:
                fx->var1 += 2;
                if (fx->var1 > 0x54) {
                    StageObj_K09_CreateMine(fx);
                }
                break;
            }
            break;
        case 0x28:
            if (em->stateGroup == 4 && em->state == 0x14) {
                StageObj_K09_Place(fx);
            }
            break;
        default:
            if (em->motionId[0] == 0x429 && EmMotion_ReachedFrame(em, 0, 300.0f)) {
                StageObj_K09_Place(fx);
            }
            break;
        }
        break;
    case 1:
        if (gScene.area == fx->angle2) {
            fx->pos[1] += 100.0f;
            fx->pos[1] = 30.0f + AreaGrid2_GetFloorY(fx->pos);
        }
        if (fx->var2 != 0xFF) {
            MinePoint_SetPos(fx->var2, fx->pos);
            if (MinePoint_Count(fx->var2) == 0) {
                fx->var0++;
                MinePoint_Remove(fx->var2);
                fx->var2 = 0xFF;
            }
        } else {
            fx->var0++;
        }
        break;
    case 2:
        if (gScene.area == fx->angle2) {
            fx->pos[1] += 100.0f;
            fx->pos[1] = 30.0f + AreaGrid2_GetFloorY(fx->pos);
        }
        break;
    }
    if (gScene.area == fx->angle2) {
        *&fx->node->pos[0] = fx->pos[0];
        fx->node->pos[1] = fx->pos[1];
        fx->node->pos[2] = fx->pos[2];
        if (1.0f == em->alpha) {
            OT_AddByDepth(gDrawOt32, fx->node, 0x20, 1);
        } else {
            OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 1);
        }
    }
}

void StageObj_K09_End(StageObj *fx)
{
    fx->state++;
    if (fx->keep == 1) {
        DrawSlotB_Free(fx->slot);
        return;
    }
    DrawSlot_Free(fx->slot);
}

void StageObj_K09_Free(StageObj *fx)
{
    StageObj_Free(fx);
}

void StageObj_K09_Draw(DrawSlot *node)
{
    StageObj *fx = node->owner;
    Em *em = fx->owner;
    u8 *mdl = (u8 *) em->model;
    u8 *bones;
    f32 *sk;
    s32 *mesh;
    f32 ofs[3];
    f32 pos[3];
    f32 rot[3];
    f32 t[16];
    f32 m[16];

    if (em->visible != 0 && mdl != NULL && mdl[0] != 0) {
        if (StageObj_K09_OwnerInvisible(em) != 1 || fx->type != 0 || fx->var1 != 0) {
            LightSet_SetAmbient(em, 1);
            LightSet_FadeAmbient(em);
            bones = (u8 *) em->model->bones;
            bones += (StageObj_K09_BoneForKind(em) & 0xFF) * 0x190;
            sk = *(f32 **) ((u8 *) em->model + 0x48);
            if (fx->type == 0) {
                if ((em->type == 0x36 || em->type == 0x3C || (u32) (em->type - 0x40) <= 1 || em->type == 0x3B) && fx->var1 != 0) {
                    Key_LerpFloat((f32 *) StageObj_K09_OfsXKeys, (f32 *) &ofs[0], fx->var1);
                    Key_LerpFloat((f32 *) StageObj_K09_OfsYKeys, (f32 *) &ofs[1], fx->var1);
                    Key_LerpFloat((f32 *) StageObj_K09_OfsZKeys, (f32 *) &ofs[2], fx->var1);
                    ofs[0] *= em->scaleX;
                    ofs[1] *= em->scaleY;
                    ofs[2] *= em->scaleZ;
                    Vec_RotateY(ofs, (6.2831855f * (u16) fx->angle) / 65536.0f);
                    Vec_Add(pos, ofs, fx->pos);
                    Mtx_Scale(m, em->scaleX, em->scaleY, em->scaleZ);
                    Mtx_Translation(t, pos[0], pos[1], pos[2]);
                    Mtx_RotateY(t, (6.2831855f * (u16) fx->angle) / 65536.0f);
                    Mtx_Concat3x3(t, m);
                    Key_LerpVec3((f32 *) StageObj_K09_RotKeys, (f32 *) rot, fx->var1);
                    Mtx_SetRotXYZ(sk + 0xD8, (6.2831855f * rot[0]) / 360.0f, (6.2831855f * rot[1]) / 360.0f, (6.2831855f * rot[2]) / 360.0f);
                    Bone_UpdateWorldScaledRoot((struct EmBone *) sk, t);
                } else {
                    Mtx_Copy(sk, (f32 *) bones);
                    Mtx_Copy(sk + 0x64, (f32 *) bones);
                    Mtx_Copy(sk + 0xC8, (f32 *) (bones + 0x190));
                }
            } else {
                Mtx_Scale(m, em->scaleX, em->scaleY, em->scaleZ);
                Mtx_Translation(t, fx->pos[0], fx->pos[1], fx->pos[2]);
                Mtx_RotateY(t, (6.2831855f * (u16) fx->angle) / 65536.0f);
                Mtx_Concat3x3(t, m);
                Bone_UpdateWorldScaledRoot((struct EmBone *) sk, t);
            }
            Bone_StoreMatricesTo((struct MtxSlot *) gBoneMatrixBuf, (struct EmBone *) sk);
            mesh = (s32 *) (*(s32 *) (mdl + 0x30) + 0x8C);
            Tex_Prepare(0xA, em->modelSlot * 20 + 0xA3);
            if (mesh[0] != -1) {
                GmEm_SetMeshAlphas(em, 1, *(void **) (mdl + 0x30));
                MtxSlot_LoadList((s16 *) (gModelPartLists + em->modelSlot * 0x420 + 0x42), (struct MtxSlot *) gBoneMatrixBuf);
                Draw_ApplyStates(mesh[0x22]);
                Model_Draw(mesh[0], 0);
            }
            Draw_SetDefaultState();
        }
    }
}

s32 StageObj_K09_BoneForKind(Em *em)
{
    switch (em->type) {
    default:
        return 0x2B;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        return 0x11;
    case 0x3B:
        return 0x11;
    }
}

s32 StageObj_K09_OwnerInvisible(Em *em)
{
    switch (em->type) {
    default:
        return 0;
    case 0x3B:
        return EmChameleos_IsInvisible(em) == 1;
        break;
    }
}

void StageObj_K09_Create(Em *em)
{
    StageObj *obj;
    u8 *q;

    if ((obj = StageObj_Alloc(0)) != NULL) {
        obj->kind = 9;
        obj->update = StageObj_K09_Update;
        obj->owner = em;
        em->k09Obj = obj;
        q = (u8 *) QuestEm_FindForEm(em);
        if (q != NULL) {
            obj->timer = (s8) q[4];
        }
    }
}

void StageObj_K0A_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        StageObj_K0A_Setup(fx);
        break;
    case 1:
        StageObj_K0A_Run(fx);
        break;
    case 2:
        StageObj_K0A_End(fx);
        break;
    case 3:
        StageObj_K0A_Free(fx);
        break;
    }
}

void StageObj_K0A_Setup(StageObj *fx)
{
    EmFxPiece *p = (EmFxPiece *) fx->buf;
    Em *m = fx->owner;
    s16 n;
    s16 i;
    f32 mtx[16];

    fx->state++;
    fx->var0 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    n = gStageObjK0A_Counts[fx->type];
    switch (fx->type) {
    case 0:
        Mtx_Copy(mtx, Em_Bone(m, gStageObjK0A_Bones[fx->var2].bone));
        fx->angle = Mtx_Heading(mtx) + 0x4000;
        fx->pos[0] = mtx[12];
        fx->pos[1] = m->floorY;
        fx->pos[2] = mtx[14];
        break;
    }
    for (i = 0; i < n; i++, p++) {
        p->slot = DrawSlot_Alloc();
        p->idx = i;
        p->unk28 = 0;
        p->unk20 = fx->scale;
        switch (fx->type) {
        case 0:
            p->unk2A = Rand_Next(1);
            p->unk2C = (Rand_Next(1) & 0xFF) - 0x80;
            Vec_Copy(p->pos, fx->pos);
            break;
        }
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = fx;
            p->node->part = i;
            p->node->draw = StageObj_K0A_Draw;
        } else {
            p->node = NULL;
        }
    }
}

/* Level 3 (the file is at 4: 26 words there). tbl indexed (MWCC's walker is set
   after the loop guard), and lim reused as the case's key index (a later web,
   numbered after the walker: j in s0, the walker in s1). */
#pragma optimization_level 3
void StageObj_K0A_Run(StageObj *fx)
{
    EmFxPiece *p;
    s16 *tbl;
    s16 n;
    s32 lim;
    s16 i;
    s16 idx;
    s16 step;
    s16 end;
    f32 v[3];

    p = (EmFxPiece *) fx->buf;
    n = gStageObjK0A_Counts[fx->type];
    lim = gStageObjK0A_Limits[fx->type];
    idx = D_008F5CC0[fx->type];
    step = gStageObjK0A_Steps[fx->type];
    end = gStageObjK0A_Ends[fx->type];
    tbl = D_008F5CC8[fx->type];

    if (++fx->timer > lim) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    for (i = 0; i < n; i++, p++) {
        if (tbl != NULL) {
            end = tbl[i];
        } else {
            idx = D_008F5CC0[fx->type]; /* again */
        }
        if (++p->unk28 <= 0) {
            idx += step;
            continue;
        }
        if (p->unk28 > end) {
            idx += step;
            continue;
        }
        switch (fx->type) {
        case 0:
            lim = idx;
            Key_LerpVec3(StageObj_K0A_Keys0[lim], p->unk10, p->unk28);
            Key_LerpVec3(StageObj_K0A_Keys0[(s16) (lim + 1)], v, p->unk28);
            v[0] += gStageObjK0A_Bones[fx->var2].v[0];
            v[1] += gStageObjK0A_Bones[fx->var2].v[1];
            v[2] += gStageObjK0A_Bones[fx->var2].v[2];
            if (p->idx != 0) {
                v[0] = -v[0];
            }
            Vec_RotateY(v, (6.2831855f * fx->angle) / 65536.0f);
            idx += 3;
            Key_LerpFloat(StageObj_K0A_Keys0[(s16) (lim + 2)], &p->unk1C, p->unk28);
            p->unk2A = p->unk2A + p->unk2C;
            break;
        }
        if (p->node != NULL) {
            p->node->pos[0] = p->pos[0] + v[0];
            p->node->pos[1] = p->pos[1] + v[1];
            p->node->pos[2] = p->pos[2] + v[2];
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
    }
}
#pragma optimization_level reset

void StageObj_K0A_End(StageObj *fx)
{
    EmFxPiece *p = (EmFxPiece *) fx->buf;
    s16 n;
    s16 i;

    fx->state++;
    n = gStageObjK0A_Counts[fx->type];
    if (fx->node != NULL) {
        DrawSlot_Free(fx->slot);
    }
    for (i = 0; i < n; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void StageObj_K0A_Free(StageObj *fx)
{
    StageObj_Free(fx);
}

void StageObj_K0A_Draw(DrawSlot *node)
{
    StageObj *fx = node->owner;
    EmFxPiece *pc = (EmFxPiece *) fx->buf + node->part;
    ModelMat *tex;
    ModelMatSet *mesh;
    Model *mdl = gCommonModels[0];
    u32 color;
    s32 order;
    f32 m[16];
    f32 scale[3];
    f32 rot[3];
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    if (mdl != NULL && mdl->active != 0) {
        tex = mdl->mats;
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        scale[0] = pc->unk20 * pc->unk10[0];
        scale[1] = pc->unk20 * pc->unk10[1];
        scale[2] = pc->unk20 * pc->unk10[2];
        switch (fx->type) {
        case 0:
            mesh = &mdl->matSets[74];
            rot[2] = (6.2831855f * pc->unk2A) / 65536.0f;
            order = 3;
            color = ((u8) (a * pc->unk1C) << 24) | (r << 16) | (g << 8) | b;
            break;
        }
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
        Mtx_Concat3x3(m, gCamMatrix);
        Effect_DrawModel(mesh, m, color, 0, tex);
    }
}

void StageObj_K0A_Create(Em *em, u8 k, u8 c, f32 f)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = 0xA;
            obj->update = StageObj_K0A_Update;
            obj->owner = em;
            obj->type = k;
            obj->var2 = c;
            obj->scale = f;
            obj->node = NULL;
        }
    }
}

void StageObj_K0B_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        StageObj_K0B_Setup(fx);
        break;
    case 1:
        StageObj_K0B_Run(fx);
        break;
    case 2:
        StageObj_K0B_End(fx);
        break;
    case 3:
        StageObj_K0B_Free(fx);
        break;
    }
}

void StageObj_K0B_Setup(StageObj *fx)
{

    fx->state++;
    fx->var0 = 0;
    fx->visible = 1;
    fx->draw = 0;
    fx->timer = 0;
    fx->var0 = (s32) Rand_Next(1);
    fx->var1 = (s32) Rand_Next(1);
    fx->var2 = (s32) Rand_Next(1);
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = StageObj_K0B_Draw;
    } else {
        StageObj_K0B_Free(fx);
        return;
    }
}

void StageObj_K0B_Run(StageObj *fx)
{
    DrawSlot *node = fx->node;
    Em *m = fx->owner;

    if (++fx->timer >= 11) {
        fx->state++;
    } else {
        Em_GetBonePos(m, 10, fx->pos);
        Vec_Copy(node->pos, (f32 *) fx->pos);
        OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
    }
}

void StageObj_K0B_End(StageObj *fx)
{
    fx->state++;
    if (fx->node != NULL) {
        DrawSlot_Free(fx->slot);
    }
}

void StageObj_K0B_Free(StageObj *fx)
{
    StageObj_Free(fx);
}

void StageObj_K0B_Draw(DrawSlot *node)
{
    StageObj *fx;
    ModelMat *tex;
    Model *mdl;
    Model *mdl2;
    Em *em;
    s16 n;
    s32 i;
    s32 j;
    s8 k;
    ModelMatSet *mesh;
    ModelMat *tex2;
    ModelMatSet *mesh2;
    s16 d;
    f32 s;
    f32 sx;
    f32 sy;
    f32 sz;
    s32 rx;
    u16 rz;
    f32 m[16];
    f32 t[16];

    fx = node->owner;
    em = fx->owner;
    if (gScene.area != em->area) {
        return;
    }
    mdl = gCommonModels[0];
    if (mdl == NULL || em->visible == 0) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x6C, 0);
    Gfx_SetTexFilter(1);
    n = fx->timer - 1;
    tex = mdl->mats;
    if (n <= 0) {
        n = 0;
    }
    Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
    Gs_SetState(0x19, (s32) t);
    i = (s16) n;
    if (i < 5) {
        mesh = &mdl->matSets[66];
        s = 10.0f + 2.5f * n;
        Mtx_Scale(m, s, s, 1.0f);
        Mtx_RotateZ(m, (6.2831855f * (fx->var0 << 8)) / 65536.0f);
        Mtx_Concat3x3(m, gCamMatrix);
        Mtx_SetTrans(m, fx->pos[0], fx->pos[1], fx->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, (u8) (0xEF - i * 0xB) | (((u8) (0xFF - i * 8) << 8) | (((u8) (0xFF - i * 0x3B) << 24) | 0xFF0000)));
        Gs_SetLightList(tex, mesh);
        Draw_ApplyStates(mesh->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(mesh->id, 0);
    }
    if (i >= 2 && i < 6) {
        mesh = mdl->matSets;
        d = i - 2;
        Mtx_Scale(m, 0.4f + 2.5333333f * d, 4.0f - d, 1.0f);
        Mtx_RotateZ(m, (6.2831855f * (fx->var1 << 8)) / 65536.0f);
        Mtx_Concat3x3(m, gCamMatrix);
        Mtx_SetTrans(m, fx->pos[0], fx->pos[1], fx->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, 0xFFFFFFEF);
        Gs_SetLightList(tex, mesh);
        Draw_ApplyStates(mesh->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(mesh->id, 0);
    }
    if (i >= 3 && i < 7) {
        mesh = mdl->matSets;
        d = i - 3;
        Mtx_Scale(m, 0.4f + 2.5333333f * d, 4.0f - d, 1.0f);
        Mtx_RotateZ(m, (6.2831855f * (fx->var2 << 8)) / 65536.0f);
        Mtx_Concat3x3(m, gCamMatrix);
        Mtx_SetTrans(m, fx->pos[0], fx->pos[1], fx->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, 0xFFFFFFEF);
        Gs_SetLightList(tex, mesh);
        Draw_ApplyStates(mesh->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(mesh->id, 0);
    }
    if (i >= 4 && i < 11) {
        k = EmSlot_FindKind(0x36);
        if (k == -1) {
            return;
        }
        mdl2 = gScene.partModels[k];
        j = (s16) (i - 4);
        tex2 = mdl2->mats;
        mesh2 = &mdl2->matSets[9];
        if (j < 3) {
            sy = sx = 0.1f + 0.10000001f * j;
            sz = 0.7f - 0.25f * j;
        } else {
            sy = sx = 0.3f + 0.024999999f * j;
            sz = 0.2f - -0.025f * j;
        }
        Mtx_Scale(m, sx, sy, sz);
        rz = (u16) (fx->var0 << 6) / 6 * i;
        rz += (rx = fx->var1 << 8);
        Mtx_RotateXYZ(m, (6.2831855f * rx) / 65536.0f, (6.2831855f * (fx->var2 << 8)) / 65536.0f, (6.2831855f * rz) / 65536.0f);
        Mtx_SetTrans(m, fx->pos[0], fx->pos[1], fx->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, (u8) (0xEF - j * 7) | (((u8) (0xFF - j * 5) << 8) | (((u8) (0xFF - j * 0x27) << 24) | 0xFF0000)));
        Gs_SetLightList(tex2, mesh2);
        Draw_ApplyStates(mesh2->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(mesh2->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(0x6C, 1);
}

void StageObj_K0B_Create(void *arg)
{
    StageObj *obj;

    if (Obj_InCurrentArea(arg) & 0xFF) {
        obj = StageObj_Alloc(0);
        if (obj != NULL) {
            obj->kind = 0xB;
            obj->update = StageObj_K0B_Update;
            obj->owner = arg;
        }
    }
}

void StageObj_K0C_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K0C_Setup(o);
        break;
    case 1:
        StageObj_K0C_Run(o);
        break;
    case 2:
        StageObj_K0C_End(o);
        break;
    case 3:
        StageObj_K0C_Free(o);
        break;
    }
}

void StageObj_K0C_Setup(StageObj *o)
{
    o->state++;
    o->var0 = 0;
    o->visible = 1;
    switch (o->type) {
    case 0:
        StageObj_K0C_SetupType0(o);
        break;
    case 1:
        StageObj_K0C_SetupType1(o);
        break;
    case 3:
        StageObj_K0C_SetupType3(o);
        break;
    case 4:
        StageObj_K0C_SetupType4(o);
        break;
    case 5:
    case 6:
        StageObj_K0C_SetupParts(o);
        break;
    default:
        StageObj_K0C_Free(o);
        break;
    }
}

void StageObj_K0C_Run(StageObj *o)
{
    switch (o->type) {
    case 0:
        StageObj_K0C_RunType0(o);
        break;
    case 1:
        StageObj_K0C_RunType1(o);
        break;
    case 3:
        StageObj_K0C_RunType3(o);
        break;
    case 4:
        StageObj_K0C_RunType4(o);
        break;
    case 5:
    case 6:
        StageObj_K0C_RunParts(o);
        
    default:
        break;
    }
}

struct DrawSlot;
void StageObj_K0C_Draw(struct DrawSlot *node)
{
    StageObj *o;
    u8 type;

    o = node->owner;
    type = o->type;
    switch (type) {
    case 0:
        StageObj_K0C_DrawType0(o);
        break;
    case 1:
        StageObj_K0C_DrawType1(o, node);
        break;
    case 3:
        StageObj_K0C_DrawType3(o);
        break;
    case 4:
        StageObj_K0C_DrawType4(o, node);
        break;
    case 5:
    case 6:
        StageObj_K0C_DrawParts(o, node);
        
    default:
        break;
    }
}

void StageObj_K0C_SetupType0(StageObj *fx)
{
    Em *m = fx->owner;

    fx->draw = 0;
    fx->timer = 0;
    Em_GetBonePos(m, 0x12, fx->pos);
    fx->pos[1] = m->floorY;
    fx->angle = m->rotY;
    fx->scale = 1.75f;
    fx->pos[1] += 13.0f * fx->scale;
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = StageObj_K0C_Draw;
    } else {
        StageObj_Free((struct StageObj *) fx);
        return;
    }
    StageObj_K0C_Run(fx);
}

void StageObj_K0C_RunType0(StageObj *o)
{
    if (o->timer++ >= 300) {
        o->state++;
        o->visible = 0;
        return;
    }
    o->node->pos[0] = o->pos[0];
    o->node->pos[1] = o->pos[1];
    o->node->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
}

void StageObj_K0C_DrawType0(StageObj *fx)
{
    Model *mdl = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *mesh;
    f32 *mat;
    s32 i;
    f32 m[16];

    if (mdl != NULL && mdl->active != 0) {
        Gfx_SetTexFilter(0);
        Mtx_Scale(m, fx->scale, fx->scale, fx->scale);
        Mtx_SetTrans(m, fx->pos[0], 30.0f + fx->pos[1], fx->pos[2]);
        Mtx_RotateY(m, (6.2831855f * (u16) fx->angle) / 65536.0f);
        Gs_SetState(0x1A, (s32) m);
        mesh = &mdl->matSets[58];
        Gs_SetState(0x67, -1);
        if (mesh != NULL && mesh->id != -1) {
            tex = mdl->mats;
            for (i = 0; i < mesh->count; i++) {
                mat = (f32 *) &tex[mesh->mat[i]];
                switch (i) {
                case 0:
                    mat[1] = 226.0f / 255.0f;
                    mat[2] = 237.0f / 255.0f;
                    mat[3] = 209.0f / 255.0f;
                    break;
                case 1:
                    mat[1] = 1.0f;
                    mat[2] = 210.0f / 255.0f;
                    mat[3] = 192.0f / 255.0f;
                    break;
                }
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags(0x10);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void StageObj_K0C_SetupType1(StageObj *fx)
{
    EmFxRing *p = (EmFxRing *) fx->buf;
    StageObjK0CBuf *buf = (StageObjK0CBuf *) fx->buf;
    s16 i;

    fx->draw = 0;
    fx->timer = 0;
    fx->angle = 0;
    switch (fx->var1) {
    case 0:
        buf->obj.radius = 1500.0f;
        buf->obj.kind = 1;
        break;
    case 1:
        buf->obj.radius = 3000.0f;
        buf->obj.kind = 2;
        break;
    }
    buf->obj.timer = 3;
    buf->obj.area = fx->angle2;
    Vec_Set(buf->obj.pos, fx->pos[0], fx->pos[1], fx->pos[2]);
    SightBlock_Add(&buf->obj);
    for (i = 0; i < 4; i++, p++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->view = i + 1;
            p->unk1C = 0;
            p->spin = Rand_Next(1);
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = fx;
            p->node->part = i;
            p->node->draw = StageObj_K0C_Draw;
        } else {
            p->node = NULL;
        }
    }
    StageObj_K0C_Run(fx);
}

extern s8 gStageObjK0CAlphaSet;
extern u8 gStageObjK0CAlpha;
void StageObj_K0C_RunType1(StageObj *fx)
{
    u8 alpha;
    u32 k;
    StageObjK0CBuf *buf = (StageObjK0CBuf *) fx->buf;
    EmFxRing *r = buf->ring;
    s16 i;
    u8 full;
    u16 *ang;
    f32 cam[3];
    f32 d[3];
    f32 a[3];
    f32 v[3];

    (fx->angle)++;
    buf->obj.timer = 3;
    switch (fx->var0) {
    case 0:
        full = 0;
        if (++fx->timer >= 120) {
            full = 1;
            fx->var0++;
            fx->timer = 0;
        }
        break;
    case 1:
        full = 1;
        if (++fx->timer >= 450) {
            full = 0;
            fx->var0++;
            fx->timer = 120;
        }
        break;
    case 2:
        full = 0;
        if (--fx->timer <= 0) {
            fx->state++;
            fx->visible = 0;
            SightBlock_Remove(&buf->obj);
        }
        break;
    }
    if (fx->visible != 0) {
        Mtx_GetTrans(cam, gCamMatrix);
        Vec_Sub(d, cam, fx->pos);
        if (!(Vec_Dot(d, d) <= 3610000.0f)) {
            for (i = 0, ang = StageObj_K0C_Angles; i < 4; i++, r++, ang++) {
                r->spin += (s32) *ang;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 400.0f * i;
                Vec_RotateSelf(v, gCamMatrix);
                if (!(full & 0xFF)) {
                    r->alpha = fx->timer / 120.0f;
                } else {
                    r->alpha = 1.0f;
                }
                if (r->node != NULL) {
                    r->view = i + 1;
                    r->node->pos[0] = fx->pos[0] + v[0];
                    r->node->pos[1] = fx->pos[1] + v[1];
                    r->node->pos[2] = fx->pos[2] + v[2];
                    OT_AddByDepth(gDrawOtSorted, r->node, 0x40, 0);
                }
            }
        } else {
            a[0] = 0.0f;
            a[1] = 0.0f;
            a[2] = -1.1f;
            Vec_Rotate(v, a, gCamMatrix);
            alpha = full == 0 ? (u32) (2.125f * fx->timer) : 0xFF;
            if (gStageObjK0CAlphaSet == 0) {
                gStageObjK0CAlpha = alpha;
                if (r->node != NULL) {
                    r->view = 0;
                    Mtx_GetTrans(r->node->pos, gCamMatrix);
                    r->node->pos[0] += v[0];
                    r->node->pos[1] += v[1];
                    r->node->pos[2] += v[2];
                    OT_AddByDepth(gDrawOtSorted, r->node, 0x40, 0);
                    *(u8 *)&gStageObjK0CAlphaSet = 1;
                }
            } else if (gStageObjK0CAlpha < (alpha & 0xFF)) {
                gStageObjK0CAlpha = alpha;
            }
        }
    }
}

void StageObj_K0C_DrawType1(StageObj *fx, DrawSlot *node)
{
    EmFxRing *r = (EmFxRing *) fx->buf + node->part;
    ModelMat *tex;
    Model *mdl = gCommonModels[0];
    ModelMatSet *mesh;
    u8 alpha;
    f32 m[16];
    f32 t[16];

    if (fx->angle2 == gScene.area && mdl != NULL && mdl->active != 0) {
        tex = mdl->mats;
        Gs_SetState(0x60, 0);
        if (r->view == 0) {
            Mtx_Scale(m, 1.0f, 1.0f, 1.0f);
            alpha = gStageObjK0CAlpha;
            Mtx_Translation(t, (1.0f / 2048.0f) * (gScene.frame & 0xFF), 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) t);
            mesh = &mdl->matSets[67];
            Camera_SetFov(0.87266463f);
        } else {
            Mtx_Scale(m, StageObj_K0C_Scales[node->part], StageObj_K0C_Scales[node->part], StageObj_K0C_Scales[node->part]);
            Mtx_RotateZ(m, (6.2831855f * r->spin) / 65536.0f);
            alpha = 255.0f * r->alpha;
            Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) t);
            mesh = &mdl->matSets[77];
        }
        Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
        Mtx_Concat3x3(m, gCamMatrix);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, D_008CE762[fx->var1][0] | ((D_008CE761[fx->var1][0] << 8) | ((alpha << 24) | (StageObj_K0C_Colors[fx->var1][0] << 16))));
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(tex, mesh);
            Draw_ApplyStates(mesh->flags);
            Model_Draw(mesh->id, 0);
        }
        *(u8 *)&gStageObjK0CAlphaSet = 0;
        Draw_SetDefaultState();
        if (r->view == 0) {
            Camera_RestoreFov();
        }
    }
}

void StageObj_K0C_SetupType3(StageObj *fx)
{

    fx->var0 = 0;
    fx->draw = 0;
    fx->timer = 0;
    if (fx->keep == 0) {
        fx->slot = DrawSlot_Alloc();
    } else {
        fx->slot = DrawSlotB_Alloc();
    }
    if (fx->slot != -1) {
        if (fx->keep == 0) {
            fx->node = DrawSlot_Get(fx->slot);
        } else {
            fx->node = DrawSlotB_Get(fx->slot);
        }
        fx->node->owner = fx;
        fx->node->draw = StageObj_K0C_Draw;
    } else {
        StageObj_K0C_Free(fx);
        return;
    }
    StageObj_K0C_Run(fx);
}

void StageObj_K0C_RunType3(StageObj *o)
{
    StageObjK0CBuf *buf = (StageObjK0CBuf *) o->buf;

    o->timer++;
    if (o->timer > 5400 || buf->obj.done != 0) {
        o->state++;
        o->visible = 0;
        ObjList_Remove(&buf->obj);
        return;
    }
    o->var0 = 0;
    o->node->pos[0] = o->pos[0];
    o->node->pos[1] = o->pos[1];
    o->node->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
}

void StageObj_K0C_DrawType3(StageObj *fx)
{
    Model *mdl = gCommonModels[0];
    ModelMatSet *mesh;
    ModelMat *tex;
    s32 i;
    f32 *mat;
    u8 r0;
    u8 g0;
    u8 b0;
    u8 r1;
    u8 g1;
    u8 b1;
    f32 m[16];

    if (fx->angle2 == gScene.area && mdl != NULL && mdl->active != 0) {
        mesh = &mdl->matSets[58];
        Mtx_Translation(m, fx->pos[0], fx->pos[1], fx->pos[2]);
        r1 = StageObj_K0C_Type3R1[fx->var1][0];
        g1 = D_008CE791[fx->var1][0];
        b1 = D_008CE792[fx->var1][0];
        r0 = StageObj_K0C_Type3R0[fx->var1][0];
        g0 = D_008CE781[fx->var1][0];
        b0 = D_008CE782[fx->var1][0];
        Mtx_RotateY(m, (6.2831855f * fx->angle) / 65536.0f);
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, -1);
        if (mesh != NULL && mesh->id != -1) {
            tex = mdl->mats;
            for (i = 0; i < mesh->count; i++) {
                mat = (f32 *) &tex[mesh->mat[i]];
                switch (i) {
                case 0:
                    mat[1] = r0 / 255.0f;
                    mat[2] = g0 / 255.0f;
                    mat[3] = b0 / 255.0f;
                    break;
                case 1:
                    mat[1] = r1 / 255.0f;
                    mat[2] = g1 / 255.0f;
                    mat[3] = b1 / 255.0f;
                    break;
                }
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(mesh->flags);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void StageObj_K0C_BonePosOffset(f32 *out, f32 *m, f32 *v)
{
    Vec_RotateSelf(v, m);
    Mtx_GetTrans(out, m);
    out[0] = out[0] + v[0];
    out[1] = out[1] + v[1];
    out[2] = out[2] + v[2];
}

void StageObj_K0C_SetupType4(StageObj *fx)
{

    fx->var0 = 0;
    fx->var1 = 0;
    fx->draw = 0;
    fx->timer = 0;
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = StageObj_K0C_Draw;
        Vec_Copy(fx->node->pos, (f32 *) fx->pos);
        fx->pos[1] = ((Em *)fx->owner)->floorY;
    } else {
        StageObj_Free((struct StageObj *) fx);
        return;
    }
    StageObj_K0C_Run(fx);
}

void StageObj_K0C_RunType4(StageObj *fx)
{
    StageObjK0CCarry *buf = (StageObjK0CCarry *) fx->buf;

    fx->timer++;
    fx->var1++;
    switch (fx->var0) {
    case 0:
        fx->node->pos[1] += -0.72727275f * fx->var1;
        if (fx->node->pos[1] < fx->pos[1]) {
            switch (buf->kind) {
            case 0:
            case 1:
            case 6:
                fx->node->pos[1] = fx->pos[1];
                fx->timer = 0;
                fx->var0++;
                if (fx->angle2 == gScene.area) {
                    Sound_PlaySeAtPosChecked(1, 0x74, 0, fx->node->pos, 1, 0);
                }
                break;
            case 2:
            case 3:
            case 4:
            default:
                fx->state++;
                fx->visible = 0;
                return;
            case 5:
                fx->state++;
                fx->visible = 0;
                if (fx->angle2 == gScene.area) {
                    StageObj_K0E_CreateScaled(fx->node->pos, 0, 1.0f);
                }
                Explosion_CreateAt(fx->node->pos, 0, fx->angle2, 0);
                return;
            }
        }
        break;
    case 1:
    default:
        if (fx->timer > 10) {
            fx->state++;
            fx->visible = 0;
            return;
        }
        break;
    }
    OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
}

void StageObj_K0C_DrawType4(StageObj *fx, DrawSlot *node)
{
    Model *mdl = gCommonModels[0];
    StageObjK0CCarry *buf = (StageObjK0CCarry *) fx->buf;
    ModelMatSet *mesh;
    ModelMat *tex;
    s32 i;
    f32 *mat;
    u8 r;
    u8 g;
    u8 b;
    f32 m[16];
    f32 t[16];

    if (fx->angle2 == gScene.area && mdl != NULL && mdl->active != 0) {
        switch (fx->var0) {
        case 0:
            Mtx_SetTrans(buf->mtx, node->pos[0], node->pos[1], node->pos[2]);
            Mtx_Copy(m, buf->mtx);
            switch (buf->kind) {
            case 0:
            case 1:
            case 6:
                mesh = &mdl->matSets[133];
                Mtx_Translation(t, 0.0f, 0.0625f, 0.0f);
                Gs_SetState(0x19, (s32) t);
                r = 0xFF;
                g = 0xFF;
                b = 0xFF;
                break;
            case 2:
                mesh = &mdl->matSets[133];
                Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) t);
                r = 0xFF;
                g = 0xFF;
                b = 0xFF;
                break;
            case 3:
                mesh = &mdl->matSets[41];
                r = 0x3F;
                g = 0x3F;
                b = 0xFF;
                break;
            case 4:
                mesh = &mdl->matSets[41];
                r = 0xFF;
                g = 0x3F;
                b = 0x3F;
                break;
            case 5:
                mesh = &mdl->matSets[41];
                r = 0x3F;
                g = 0x3F;
                b = 0x3F;
                break;
            default:
                return;
            }
            break;
        case 1:
        default:
            Mtx_Scale(m, 1.5f, 1.5f, 1.5f);
            Mtx_Concat3x3(m, gCamYawMtx);
            mesh = &mdl->matSets[119];
            Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
            r = 0xEE;
            g = 0xDC;
            b = 0xA8;
            break;
        }
        Gs_SetState(0x1A, (s32) m);
        Gs_SetState(0x67, -1);
        if (mesh != NULL && mesh->id != -1) {
            tex = mdl->mats;
            for (i = 0; i < mesh->count; i++) {
                mat = (f32 *) &tex[mesh->mat[i]];
                switch (i) {
                case 1:
                    mat[1] = r / 255.0f;
                    mat[2] = g / 255.0f;
                    mat[3] = b / 255.0f;
                    break;
                }
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(mesh->flags);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* em_bulldrome's .data (0x8CB4E0-0x8CE640) */
f32 EmBulldrome_LaunchVel[4] = {
#include "game/008CB4E0.inc"
};
f32 EmBulldrome_LaunchAccel[4] = {
#include "game/008CB4F0.inc"
};
u32 gEmMotionRows_68[304] = {
#include "game/008CB500.inc"
};
u32 gEmKindTables_68_1_1[8] = {
#include "game/008CB9C0.inc"
};
u32 gEmKindTables_68_1_3[12] = {
#include "game/008CB9E0.inc"
};
u32 gEmKindTables_68_1_5[12] = {
#include "game/008CBA10.inc"
};
u32 gEmKindTables_68_1_7[8] = {
#include "game/008CBA40.inc"
};
u32 gEmKindTables_68_1_9[8] = {
#include "game/008CBA60.inc"
};
u32 gEmKindTables_68_1_11[12] = {
#include "game/008CBA80.inc"
};
u32 gEmKindTables_68_1_13[16] = {
#include "game/008CBAB0.inc"
};
u32 gEmKindTables_68_1_15[16] = {
#include "game/008CBAF0.inc"
};
u32 gEmKindTables_68_1_17[12] = {
#include "game/008CBB30.inc"
};
u32 gEmKindTables_68_1_19[10] = {
#include "game/008CBB60.inc"
};
u32 gEmKindTables_68_1_21[3] = {
#include "game/008CBB88.inc"
};
u32 gEmKindTables_68_1_23[8] = {
#include "game/008CBBA0.inc"
};
u32 gEmKindTables_68_1_25[12] = {
#include "game/008CBBC0.inc"
};
u32 gEmKindTables_68_1[28] = {
#include "game/008CBBF0.inc"
};
u32 gEmKindTables_68_3[2] = {
#include "game/008CBC60.inc"
};
u32 gEmKindTables_68_4[2] = {
#include "game/008CBC68.inc"
};
u32 gEmKindTables_68_5[4] = {
#include "game/008CBC70.inc"
};
u32 gEmKindTables_68_6_3_1[16] = {
#include "game/008CBC80.inc"
};
u32 gEmKindTables_68_6_3_7[16] = {
#include "game/008CBCC0.inc"
};
u32 gEmKindTables_68_6_3[12] = {
#include "game/008CBD00.inc"
};
u32 gEmKindTables_68_6_1_1[12] = {
#include "game/008CBD30.inc"
};
u32 gEmKindTables_68_6_1_7[20] = {
#include "game/008CBD60.inc"
};
u32 gEmKindTables_68_6_1[12] = {
#include "game/008CBDB0.inc"
};
u32 gEmKindTables_68_6_5_1[20] = {
#include "game/008CBDE0.inc"
};
u32 gEmKindTables_68_6_5[8] = {
#include "game/008CBE30.inc"
};
u32 gEmKindTables_68_6_9_1[28] = {
#include "game/008CBE50.inc"
};
u32 gEmKindTables_68_6_9[8] = {
#include "game/008CBEC0.inc"
};
u32 gEmKindTables_68_6_7_1[16] = {
#include "game/008CBEE0.inc"
};
u32 gEmKindTables_68_6_7[8] = {
#include "game/008CBF20.inc"
};
u32 gEmKindTables_68_6_11_1[16] = {
#include "game/008CBF40.inc"
};
u32 gEmKindTables_68_6_11_7[20] = {
#include "game/008CBF80.inc"
};
u32 gEmKindTables_68_6_11[12] = {
#include "game/008CBFD0.inc"
};
u32 D_008CC000[16] = {
#include "game/008CC000.inc"
};
u32 gEmKindTables_68_6_13_7[16] = {
#include "game/008CC040.inc"
};
u32 gEmKindTables_68_6_13[12] = {
#include "game/008CC080.inc"
};
u32 gEmKindTables_68_6_15_1[16] = {
#include "game/008CC0B0.inc"
};
u32 gEmKindTables_68_6_15_7[16] = {
#include "game/008CC0F0.inc"
};
u32 gEmKindTables_68_6_15[12] = {
#include "game/008CC130.inc"
};
u32 gEmKindTables_68_6_17_1[20] = {
#include "game/008CC160.inc"
};
u32 gEmKindTables_68_6_17_7[20] = {
#include "game/008CC1B0.inc"
};
u32 gEmKindTables_68_6_17[12] = {
#include "game/008CC200.inc"
};
u32 gEmKindTables_68_6_19_1[16] = {
#include "game/008CC230.inc"
};
u32 gEmKindTables_68_6_19_7[20] = {
#include "game/008CC270.inc"
};
u32 gEmKindTables_68_6_19_13[16] = {
#include "game/008CC2C0.inc"
};
u32 gEmKindTables_68_6_19[20] = {
#include "game/008CC300.inc"
};
u32 gEmKindTables_68_6_21_1[16] = {
#include "game/008CC350.inc"
};
u32 gEmKindTables_68_6_21_7[12] = {
#include "game/008CC390.inc"
};
u32 gEmKindTables_68_6_21[12] = {
#include "game/008CC3C0.inc"
};
u32 gEmKindTables_68_6_23_1[16] = {
#include "game/008CC3F0.inc"
};
u32 gEmKindTables_68_6_23[8] = {
#include "game/008CC430.inc"
};
u32 gEmKindTables_68_6_25_1[16] = {
#include "game/008CC450.inc"
};
u32 gEmKindTables_68_6_25_7[16] = {
#include "game/008CC490.inc"
};
u32 gEmKindTables_68_6_25[12] = {
#include "game/008CC4D0.inc"
};
u32 gEmKindTables_68_6[28] = {
#include "game/008CC500.inc"
};
extern u32 gEmKindTables_68_0[];
extern u32 gEmKindTables_68_2[];
u32 gEmKindTables_68[8] = {
    (u32)gEmKindTables_68_0, (u32)gEmKindTables_68_1, (u32)gEmKindTables_68_2, (u32)gEmKindTables_68_3,
    (u32)gEmKindTables_68_4, (u32)gEmKindTables_68_5, (u32)gEmKindTables_68_6, 0,
};
u32 gEmAreaTimer1Values_68[4] = {
#include "game/008CC590.inc"
};
u32 gEmAreaTimer2Values_68[16] = {
#include "game/008CC5A0.inc"
};
EmCurveKey EmFatalis_MoveVelYCurves_1[17] = {
#include "game/008CC5E0.inc"
};
EmCurveKey *EmFatalis_MoveVelYCurves[2] = {
    NULL, EmFatalis_MoveVelYCurves_1,
};
EmCurveKey EmFatalis_VelYCurves_0[38] = {
#include "game/008CC670.inc"
};
EmCurveKey EmFatalis_VelYCurves_2[26] = {
#include "game/008CC7A0.inc"
};
EmCurveKey EmFatalis_VelYCurves_3[30] = {
#include "game/008CC870.inc"
};
EmCurveKey EmFatalis_VelYCurves_5[10] = {
#include "game/008CC960.inc"
};
EmCurveKey EmFatalis_VelYCurves_6[42] = {
#include "game/008CC9B0.inc"
};
EmCurveKey *EmFatalis_VelYCurves[8] = {
    EmFatalis_VelYCurves_0, NULL, EmFatalis_VelYCurves_2, EmFatalis_VelYCurves_3,
    NULL, EmFatalis_VelYCurves_5, EmFatalis_VelYCurves_6, NULL,
};
EmCurveKey EmFatalis_SpeedCurves_3[14] = {
#include "game/008CCB20.inc"
};
EmCurveKey EmFatalis_SpeedCurves_4[10] = {
#include "game/008CCB90.inc"
};
EmCurveKey EmFatalis_SpeedCurves_5[10] = {
#include "game/008CCBE0.inc"
};
EmCurveKey *EmFatalis_SpeedCurves[8] = {
    NULL, NULL, NULL, EmFatalis_SpeedCurves_3,
    EmFatalis_SpeedCurves_4, EmFatalis_SpeedCurves_5, NULL, NULL,
};
EmFxOfs EmFxSeg_OffsetL[32] = {
#include "game/008CCC50.inc"
};
EmFxOfs EmFxSeg_OffsetR[32] = {
#include "game/008CCD50.inc"
};
EmFxOfs EmFxSeg_ScaleL[32] = {
#include "game/008CCE50.inc"
};
EmFxOfs EmFxSeg_ScaleR[31] = {
#include "game/008CCF50.inc"
};
s16 EmFxSeg_Counts[4] = {
#include "game/008CD048.inc"
};
EmFxBoneOfs EmFxSpark_Bones[10] = {
#include "game/008CD050.inc"
};
u32 EmFxSpark_Curves_0[20] = {
#include "game/008CD120.inc"
};
u32 EmFxSpark_Curves_2[12] = {
#include "game/008CD170.inc"
};
u32 EmFxSpark_Curves_1[8] = {
#include "game/008CD1A0.inc"
};
u32 EmFxSpark_Curves_3[8] = {
#include "game/008CD1C0.inc"
};
s16 EmFxSpark_CurveSets_1[8] = {
#include "game/008CD1E0.inc"
};
u32 EmFxSpark_Curves_4[16] = {
#include "game/008CD1F0.inc"
};
u32 EmFxSpark_Curves_5[12] = {
#include "game/008CD230.inc"
};
u32 EmFxSpark_Curves_6[20] = {
#include "game/008CD260.inc"
};
u32 EmFxSpark_Curves_7[12] = {
#include "game/008CD2B0.inc"
};
u32 EmFxSpark_Curves_8[12] = {
#include "game/008CD2E0.inc"
};
u32 EmFxSpark_Curves_9[12] = {
#include "game/008CD310.inc"
};
u8 EmFxSpark_Curve0[32] = {
#include "game/008CD340.inc"
};
u8 EmFxSpark_Curve1[32] = {
#include "game/008CD360.inc"
};
u8 EmFxSpark_Curve2[32] = {
#include "game/008CD380.inc"
};
u8 EmFxSpark_Curve3[32] = {
#include "game/008CD3A0.inc"
};
s16 EmFxSpark_CurveSets_3[8] = {
#include "game/008CD3C0.inc"
};
u32 EmFxSpark_Curves_11[24] = {
#include "game/008CD3D0.inc"
};
u32 EmFxSpark_Curves_13[28] = {
#include "game/008CD430.inc"
};
u32 EmFxSpark_Curves_14[20] = {
#include "game/008CD4A0.inc"
};
u32 EmFxSpark_Curves_15[16] = {
#include "game/008CD4F0.inc"
};
u32 EmFxSpark_Curves_16[10] = {
#include "game/008CD530.inc"
};
s16 EmFxSpark_CurveSets_4[6] = {
#include "game/008CD558.inc"
};
u8 EmFxSpark_KindCurves5_0[32] = {
#include "game/008CD570.inc"
};
u8 EmFxSpark_KindCurves5_2[32] = {
#include "game/008CD590.inc"
};
u8 EmFxSpark_KindCurves5_3[48] = {
#include "game/008CD5B0.inc"
};
u8 EmFxSpark_KindCurves5_4[32] = {
#include "game/008CD5E0.inc"
};
u8 *EmFxSpark_KindCurves5[8] = {
    EmFxSpark_KindCurves5_0, NULL, EmFxSpark_KindCurves5_2, EmFxSpark_KindCurves5_3,
    EmFxSpark_KindCurves5_4, NULL, NULL, NULL,
};
u32 EmFxSpark_Curves_17[20] = {
#include "game/008CD620.inc"
};
u32 EmFxSpark_Curves_19[18] = {
#include "game/008CD670.inc"
};
s16 EmFxSpark_CurveSets_5[4] = {
#include "game/008CD6B8.inc"
};
u8 EmFxSpark_KindCurves1_0[32] = {
#include "game/008CD6C0.inc"
};
u8 EmFxSpark_KindCurves1_1[32] = {
#include "game/008CD6E0.inc"
};
u8 *EmFxSpark_KindCurves1[4] = {
    EmFxSpark_KindCurves1_0, EmFxSpark_KindCurves1_1, NULL, NULL,
};
u32 EmFxSpark_Curves_20[20] = {
#include "game/008CD710.inc"
};
u32 EmFxSpark_Curves_21[22] = {
#include "game/008CD760.inc"
};
s16 EmFxSpark_CurveSets_6[4] = {
#include "game/008CD7B8.inc"
};
u32 EmFxSpark_Curves_22[24] = {
#include "game/008CD7C0.inc"
};
u32 EmFxSpark_Curves_24[16] = {
#include "game/008CD820.inc"
};
u32 EmFxSpark_Curves_26[20] = {
#include "game/008CD860.inc"
};
u32 EmFxSpark_Curves_23[8] = {
#include "game/008CD8B0.inc"
};
u32 EmFxSpark_Curves_25[8] = {
#include "game/008CD8D0.inc"
};
u32 EmFxSpark_Curves_27[8] = {
#include "game/008CD8F0.inc"
};
s16 EmFxSpark_CurveSets_7[4] = {
#include "game/008CD910.inc"
};
s16 EmFxSpark_CurveSets_8[4] = {
#include "game/008CD918.inc"
};
void *EmFxSpark_Curves[28] = {
    (void *)EmFxSpark_Curves_0, (void *)EmFxSpark_Curves_1, (void *)EmFxSpark_Curves_2, (void *)EmFxSpark_Curves_3,
    (void *)EmFxSpark_Curves_4, (void *)EmFxSpark_Curves_5, (void *)EmFxSpark_Curves_6, (void *)EmFxSpark_Curves_7,
    (void *)EmFxSpark_Curves_8, (void *)EmFxSpark_Curves_9, (void *)EmFxSpark_Curves_9, (void *)EmFxSpark_Curves_11,
    (void *)EmFxSpark_Curves_11, (void *)EmFxSpark_Curves_13, (void *)EmFxSpark_Curves_14, (void *)EmFxSpark_Curves_15,
    (void *)EmFxSpark_Curves_16, (void *)EmFxSpark_Curves_17, (void *)EmFxSpark_Curves_17, (void *)EmFxSpark_Curves_19,
    (void *)EmFxSpark_Curves_20, (void *)EmFxSpark_Curves_21, (void *)EmFxSpark_Curves_22, (void *)EmFxSpark_Curves_23,
    (void *)EmFxSpark_Curves_24, (void *)EmFxSpark_Curves_25, (void *)EmFxSpark_Curves_26, (void *)EmFxSpark_Curves_27,
};
s16 EmFxSpark_Counts[16] = {
#include "game/008CD990.inc"
};
s16 EmFxSpark_Durations[16] = {
#include "game/008CD9B0.inc"
};
s16 EmFxSpark_BoneIdx[16] = {
#include "game/008CD9D0.inc"
};
s16 EmFxSpark_BoneIdx2[16] = {
#include "game/008CD9F0.inc"
};
s16 *EmFxSpark_CurveSets[12] = {
    NULL, EmFxSpark_CurveSets_1, NULL, EmFxSpark_CurveSets_3,
    EmFxSpark_CurveSets_4, EmFxSpark_CurveSets_5, EmFxSpark_CurveSets_6, EmFxSpark_CurveSets_7,
    EmFxSpark_CurveSets_8, NULL, NULL, NULL,
};
f32 StageObj_K05_Weights3[6][3] = {
#include "game/008CDA40.inc"
};
f32 StageObj_K05_Weights2[6][2] = {
#include "game/008CDA90.inc"
};
EmFxVec3 StageObj_K05_Offsets[10] = {
#include "game/008CDAC0.inc"
};
EmFxVec3 StageObj_K05_OffsetsB[32] = {
#include "game/008CDB40.inc"
};
EmFxVec3 StageObj_K05_OffsetsC[32] = {
#include "game/008CDCC0.inc"
};
f32 StageObj_K05_ScalesA[32] = {
#include "game/008CDE40.inc"
};
f32 StageObj_K05_ScalesB[32] = {
#include "game/008CDEC0.inc"
};
f32 EmFxTri_Blink0[2] = {
#include "game/008CDF40.inc"
};
f32 EmFxTri_Blink1[2] = {
#include "game/008CDF48.inc"
};
f32 EmFxTri_ScaleKeys2[8] = {
#include "game/008CDF50.inc"
};
u8 EmFxTri_ColorKeys2[32] = {
#include "game/008CDF70.inc"
};
f32 EmFxTri_Offsets3[6][3] = {
#include "game/008CDF90.inc"
};
f32 EmFxTri_Vecs5[4][3] = {
#include "game/008CDFE0.inc"
};
u8 StageObj_K09_OfsXKeys[48] = {
#include "game/008CE010.inc"
};
u8 StageObj_K09_OfsYKeys[304] = {
#include "game/008CE040.inc"
};
u8 StageObj_K09_OfsZKeys[144] = {
#include "game/008CE170.inc"
};
u8 StageObj_K09_RotKeys[704] = {
#include "game/008CE200.inc"
};
u32 StageObj_K0A_Keys0_0[24] = {
#include "game/008CE4C0.inc"
};
u32 StageObj_K0A_Keys0_1[24] = {
#include "game/008CE520.inc"
};
u32 StageObj_K0A_Keys0_2[8] = {
#include "game/008CE580.inc"
};
EmFxBone gStageObjK0A_Bones[5] = {
#include "game/008CE5A0.inc"
};
void *StageObj_K0A_Keys0[4] = {
    (void *)StageObj_K0A_Keys0_0, (void *)StageObj_K0A_Keys0_1, (void *)StageObj_K0A_Keys0_2, NULL,
};
s16 gStageObjK0A_Counts[4] = {
#include "game/008CE600.inc"
};
s16 gStageObjK0A_Limits[4] = {
#include "game/008CE608.inc"
};
s16 gStageObjK0A_Steps[4] = {
#include "game/008CE610.inc"
};
s16 gStageObjK0A_Ends[4] = {
#include "game/008CE618.inc"
};
f32 StageObj_K0C_Scales[4] = {
#include "game/008CE620.inc"
};
u32 StageObj_K0C_PlayerVals[2] = {
#include "game/008CE630.inc"
};
u16 StageObj_K0C_Angles[4] = {
#include "game/008CE638.inc"
};
