/*
 * Game overlay code at 0x701810-0x7077A0, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"

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

extern s16 AreaObj_K17_AngleTbl[];
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
extern EmMotionRow *gEmMotionRows[];
extern EmMotionRow *gEmMotionAltRows[];
extern EmMotionRow *gEmMotionTransRows[];
extern f32 *gEmSenseLimits[];
s8 Em_AngleSector(Em *em, s32 idx);
extern s32 (**gEmPlayerActionKinds[])[4];
void Em_AddAlertBuildup(Em *em, u8 player, u8 idx, s8 kind);
extern s32 *gEmNoticeDecayRates[];
s32 EmBlangonga_PartFlagUnset(Em *em, u16 a);
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
u8 AreaHit_Raycast(f32 *from, f32 *to, f32 *out, u16 k);
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
u8 Quest_GetSeedByte(u8 k);
void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void Eff37_Create(f32 *pos, u8 a, u8 b);
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
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), Em_NearestActionToBone: 26 of 86 words differ */
void Em_BonePos(void *obj, s32 bone, f32 *pos);
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
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_khezu.c), Em_ChangeState: 63 of 131 words differ */
void EmAnteka_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_plesioth.c), Em_ChangeState: 56 of 131 words differ */
void EmApceros_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_blangonga.c), Em_ChangeState: 43 of 131 words differ */
void EmBlangonga_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_bulldrome.c), Em_ChangeState: 35 of 131 words differ */
void EmBulldrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_cephadrome.c), Em_ChangeState: 59 of 131 words differ */
void EmCephadrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_chameleos.c), Em_ChangeState: 38 of 131 words differ */
void EmChameleos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_diablos.c), Em_ChangeState: 54 of 131 words differ */
void EmDiablos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_drome.c), Em_ChangeState: 50 of 131 words differ */
void EmDrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_bulldrome.c), Em_ChangeState: 64 of 131 words differ */
void EmFatalis_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_garuga.c), Em_ChangeState: 45 of 131 words differ */
void EmGaruga_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_gravios.c), Em_ChangeState: 52 of 131 words differ */
void EmGravios_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_hermitaur.c), Em_ChangeState: 44 of 131 words differ */
void EmHermitaur_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_khezu.c), Em_ChangeState: 53 of 131 words differ */
void EmKhezu_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_drome.c), Em_ChangeState: 46 of 131 words differ */
void EmKirin_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_blangonga.c), Em_ChangeState: 42 of 131 words differ */
void EmKushala_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_kutku.c), Em_ChangeState: 61 of 131 words differ */
void EmKutku_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_laoshan.c), Em_ChangeState: 60 of 131 words differ */
void EmLaoshan_ChangeState(Em *em, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_cephadrome.c), Em_ChangeState: 58 of 131 words differ */
void EmMelynx_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_khezu.c), Em_ChangeState: 62 of 131 words differ */
void EmMosswine_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_plesioth.c), Em_ChangeState: 51 of 131 words differ */
void EmPlesioth_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_rathalos.c), Em_ChangeState: 65 of 131 words differ */
void EmRathalos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_chameleos.c), Em_ChangeState: 37 of 131 words differ */
void EmRemobra_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_kushala.c), Em_ChangeState: 57 of 131 words differ */
void EmShakalaka_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_blangonga.c), Em_ChangeState: 41 of 131 words differ */
void EmShenGaoren_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_khezu.c), Em_ChangeState: 55 of 131 words differ */
void EmVelociprey_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_gravios.c), Em_ChangeState: 49 of 131 words differ */
void EmVespoid_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_kushala.c), Em_ChangeState: 39 of 131 words differ */
void EmYamaTsukami_ChangeState(Em *, s32, s32, s32);
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
void Em_BendStep(Em *em, u32 speed, s32 angle);
void Em_SetState(Em *em, s32 group, s32 state);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ComputeGoal2(Em *em, f32 *target);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_Add7F0(Em *em, s32 n);
void Em_UseGoalAction(Em *em);
/* unit's own declaration: with the definition's void (Em *em, u16 group, u16 state, u8 mode) (game/em_kushala.c), Em_ChangeState: 40 of 131 words differ */
void EmKind38_ChangeState(Em *, s32, s32, s32);
s32 Sprite_Draw(Sprite *s);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetState63(s32 a);
void Tex_Bind(s32 i);
void DrawSlot_Free(s32 id);
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
s32 Model_Draw(u32 model, s32 flags);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 a, u32 b);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern EmAction *gAreaObjList[32];
u32 Player_TestFlag(void *obj, u32 flag);
f32 Em_RageScale1(Em *em);
f32 Em_RageScale2(Em *em);
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
typedef struct EmTurnView {
    u8 pad[0x590];
    u16 target;
    u16 heading;
    u8 pad594[0x63C - 0x594];
    u32 bend[4];
    u16 speed;
    u16 hold;
    u8 phase;
} EmTurnView;
typedef struct EmBendView {
    u8 pad[0x63C];
    u32 bend[4];
} EmBendView;
extern EmBendRow *gEmBendOrigins[];
/* end of prototypes */

/* Starts `motion` on motion layer `layer` from `frame`, blending over blend / 2 frames (no blend when
 * the layer already plays it or plays nothing). The game adds the byte offset before the monster's
 * address (as in Em_AddNotice); the (u32) keeps the second address from being shared with the third
 * (the game computes each apart). */
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer)
{
    s32 off;
    u16 *m;
    s16 *b;

    if (layer < em->motionLayerCount) {
        Mtx_FromAnglesXYZ(&em->rotX, em->rotMtx[0]);
        off = layer * 2;
        m = (u16 *) (off + (s32) em + 0x29C);
        if (*m == motion || *m == 0) {
            blend = 0;
        }
        *m = motion;
        b = (s16 *) (off + (u32) em + 0x2AC);
        *b = blend / 2;
        *(u16 *) (off + (s32) em + 0x2A4) = frame;
        Player_BeginLayerMotion((Player *)em, *(u16 *) (off + (s32) em + 0x2A4), *b, layer);
    }
}

void Em_UpdateBendTarget(Em *em)
{
    f32 pos[3];
    s8 target;

    if (em->lookAtTarget == 0) {
        Em_TurnToward(em, gPlayers[0].pos, 0);
        return;
    }
    if (*(s8 *)&em->lookAtPoint != 0) { /* read as s8 here */
        Em_TurnToward(em, em->lookPoint, 1);
        return;
    }
    target = em->targetNo;
    if (target == -1) {
        Em_TurnToward(em, gPlayers[0].pos, 0);
        return;
    }
    Em_GetTargetPos(em, target & 0xFF, pos);
    Em_TurnToward(em, pos, 1);
}

/* Turns the monster toward the point `target` (mode 1; mode 0 clears the turn): the heading at +0x592
 * follows the wanted heading at +0x590 (seen from the point gEmBendOrigins[kind] +0x0 ahead of the
 * monster) by steps of +0x64C, which phase 2 speeds up or slows down (limits at +0xA, +0xC and +0xE
 * of the kind's row) as the angle left shrinks; crossing the 0x8000 line keeps the old heading
 * (+0x64E) until the step allows. Then Em_BendStep bends the body toward it.
 * Matched in round 30 (from 45 words): the (u16) casts on tgt in case 2's heading test and on cur
 * after the switch (the game masks each once and compares the masked copies), `(u16) cur >= 0x8000`,
 * hd declared before flag. Math_Heading's result is held in cur (its first web, so that
 * cur = e->heading is a later web and takes v1 as in the game); case 2's side0 flag is kept in d
 * (dead there: a later web of d, a1 as in the game). */
void Em_TurnToward(Em *em, f32 *target, s32 mode)
{
    u16 diff;
    u16 spd;
    EmTurnView *e = (EmTurnView *) em;
    u16 base = gEmBendOrigins[em->type]->turnSpeed;
    u16 max = gEmBendOrigins[em->type]->turnSpeedMax;
    u16 acc = gEmBendOrigins[em->type]->turnAccel;
    EmBendRow *tbl = gEmBendOrigins[em->type];
    s32 cur;
    s32 tgt;
    u16 hd;
    u16 flag;
    s32 v;
    u16 d;
    u16 side1;
    u16 hold;
    u16 th;
    f32 p[3];
    f32 ofs[3];
    if (mode == 0) {
        e->target = 0;
        e->hold = 0;
        e->phase = 0;
    } else {
        p[0] = 0.0f;
        p[1] = 0.0f;
        p[2] = tbl->ahead;
        Vec_Rotate(ofs, p, (f32 *) em->rotMtx);
        p[0] = em->posX + ofs[0];
        p[2] = em->posZ + ofs[2];
        cur = (u16) Math_Heading(p[0], p[2], target[0], target[2]);
        e->target = cur - 0x4000 - em->rotY;
        if (e->phase == 0) {
            e->phase = 1;
        }
    }
    cur = e->heading;
    tgt = e->target;
    flag = 0;
    diff = tgt - cur;
    switch (e->phase) {
    case 0:
        e->hold = tgt;
        e->speed = base;
        spd = base;
        break;
    case 1:
        d = diff;
        e->hold = tgt;
        e->speed = base;
        if (d > 0x8000) {
            d = 0x10000 - d;
        }
        if (em->type == 0x3B && d < 0xE39) {
            spd = 0;
            break;
        }
        if (d >= tbl->slowAngle / 4) {
            spd = base;
            e->phase = 2;
        } else if (d >= tbl->slowAngle / 8) {
            spd = base;
            e->phase = 3;
        } else {
            spd = base;
        }
        break;
    case 2:
        hold = e->hold;
        hd = hold - cur;
        d = 0;
        if (hold <= 0x8000 && cur >= 0x8000) {
            d = 1;
        }
        if ((u16) cur <= 0x8000 && hold >= 0x8000) {
            d = 2;
        }
        side1 = 0;
        if ((u16) tgt <= 0x8000 && cur >= 0x8000) {
            side1 = 1;
        }
        if (cur <= 0x8000 && (u16) tgt >= 0x8000) {
            side1 = 2;
        }
        if (side1 == d) {
            e->hold = tgt;
        } else {
            switch (side1) {
            case 0:
                if ((u16) (diff + e->speed) < (u32) (e->speed * 2)) {
                    if (side1 == d) {
                        e->hold = tgt;
                    } else {
                        tgt = hold;
                        diff = hd;
                        flag = 1;
                    }
                } else {
                    if (side1 == d) {
                        e->hold = tgt;
                    } else {
                        tgt = hold;
                        diff = hd;
                        flag = 1;
                    }
                }
                break;
            case 1:
            case 2:
                th = tbl->slowAngle;
                if ((((u16) tgt < th && cur > 0x10000 - th) || (cur < th && 0x10000 - th < (u16) tgt))
                    && (u16) (diff + e->speed) < (u32) (e->speed * 2)) {
                    if (side1 == d) {
                        e->hold = tgt;
                    } else {
                        tgt = hold;
                        diff = hd;
                        flag = 1;
                    }
                } else {
                    if (side1 == d) {
                        e->hold = tgt;
                    } else {
                        tgt = hold;
                        diff = hd;
                        flag = 1;
                    }
                }
                break;
            }
        }
        d = diff;
        if (d > 0x8000) {
            d = 0x10000 - d;
        }
        if (!flag) {
            th = tbl->slowAngle;
            if (d >= th) {
                if (max > e->speed) {
                    e->speed += acc;
                }
            } else if (d >= th / 2) {
                /* FAKEMATCH: the (s32) cast (max is promoted anyway) keeps MWCC from sharing max / 2
                 * between the two tests as a common subexpression; a later pass shares it instead,
                 * numbered after the speed load, so max / 2 takes v1 and the speed a0 as in the game.
                 * Without it: speed v1, max / 2 a0 (7 words); `e->speed = e->speed + acc` gives the
                 * game's registers but masks acc (4). The permuter found it as `max / ((0, 2))`. */
                if (e->speed < (s32) max / 2) {
                    e->speed += acc;
                } else if (e->speed > max / 2) {
                    e->speed -= acc;
                }
            } else if (d >= th / 4) {
                if (e->speed < (max / 8) * 3) {
                    e->speed += acc;
                } else if (e->speed > (max / 8) * 3) {
                    e->speed -= acc;
                }
            } else if (max / 4 < e->speed) {
                e->speed -= acc;
            } else {
                e->speed = base;
                e->phase = 3;
                e->hold = e->target;
                tgt = e->target;
            }
        } else if (max / 4 < e->speed) {
            e->speed -= acc;
        } else {
            e->speed = base;
            e->phase = 3;
            e->hold = e->target;
            tgt = e->target;
        }
        spd = e->speed;
        break;
    case 3:
        e->hold = tgt;
        d = diff;
        e->speed = base;
        if (d == 0) {
            spd = 0;
            e->phase = 1;
        } else {
            if (d > 0x8000) {
                d = 0x10000 - d;
            }
            if (d >= tbl->slowAngle / 4) {
                spd = base;
                e->phase = 2;
            } else {
                spd = base;
            }
        }
        break;
    }
    v = (flag) ? e->hold : e->target;
    cur = e->heading;
    side1 = 0;
    if ((u16) v <= 0x8000 && (u16) cur >= 0x8000) {
        side1 = 1;
    }
    if ((u16) cur <= 0x8000 && (u16) v >= 0x8000) {
        side1 = 2;
    }
    switch (side1) {
    case 0:
        if ((u16) (diff + spd) < spd * 2) {
            e->heading = e->target;
            Em_BendStep(em, spd, tgt);
        } else if (diff < 0x8000) {
            e->heading = (u16) cur + spd;
            Em_BendStep(em, spd, tgt);
        } else {
            e->heading = (u16) cur - spd;
            Em_BendStep(em, spd, tgt);
        }
        break;
    case 1:
    case 2:
        th = tbl->slowAngle;
        if (((v < th && (u16) cur > 0x10000 - th) || ((u16) cur < th && 0x10000 - th < v))
            && (u16) (diff + spd) < spd * 2) {
            e->heading = e->target;
            Em_BendStep(em, spd, tgt);
        } else if (side1 == 1) {
            e->heading = (u16) cur + spd;
            Em_BendStep(em, spd, tgt);
        } else {
            e->heading = (u16) cur - spd;
            Em_BendStep(em, spd, tgt);
        }
        break;
    }
    e->bend[0] = *(u16 *) &e->bend[0];
    e->bend[1] = *(u16 *) &e->bend[1];
    e->bend[2] = *(u16 *) &e->bend[2];
    e->bend[3] = *(u16 *) &e->bend[3];
}

/* Turns the four bend angles at Em +0x63C toward the angle `angle` (split over them as
 * |angle| / 2, / 4, / 8, / 16 plus |angle| / 64, each capped by the kind's limits at +0x10 of
 * gEmBendOrigins[kind], the excess carried to the next), by steps from `speed` split the same way.
 * One temporary, cur, holds |angle| and then each current bend angle (with a separate `a`,
 * blocks 2 and 3 of the negative-angle half swap two registers). */
void Em_BendStep(Em *em, u32 speed, s32 angle)
{
    EmBendView *e = (EmBendView *) em;
    EmBendRow *tbl = gEmBendOrigins[em->type];
    u32 s0;
    u32 s1;
    u32 s2;
    u32 s3;
    u32 v0;
    u32 v1;
    u32 v2;
    u32 v3;
    u32 d;
    u32 cur;
    u32 t0;
    u32 t1;
    u32 t2;
    u32 t3;

    s0 = (u16) ((speed >> 1) + (speed >> 6));
    s1 = (u16) ((speed >> 2) + (speed >> 6));
    s2 = (u16) ((speed >> 3) + (speed >> 6));
    s3 = (u16) ((speed >> 4) + (speed >> 6));
    cur = (u16) angle;
    if (cur > 0x8000) {
        cur = 0x10000 - cur;
    }
    v0 = (u16) ((cur >> 1) + (cur >> 6));
    v1 = (u16) ((cur >> 2) + (cur >> 6));
    v2 = (u16) ((cur >> 3) + (cur >> 6));
    v3 = (u16) ((cur >> 4) + (cur >> 6));
    if (tbl->bendMax[0] < v0) {
        v1 += v0 - tbl->bendMax[0];
        v0 = tbl->bendMax[0];
    }
    if (tbl->bendMax[1] < v1) {
        v2 += v1 - tbl->bendMax[1];
        v1 = tbl->bendMax[1];
    }
    if (tbl->bendMax[2] < v2) {
        v3 += v2 - tbl->bendMax[2];
        v2 = tbl->bendMax[2];
    }
    if (tbl->bendMax[3] < v3) {
        v3 = tbl->bendMax[3];
    }
    if ((u16) angle < 0x8000) {
        cur = e->bend[0];
        if (cur > 0x8000) {
            d = v0 + (0x10000 - cur);
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        } else {
            d = v0 - cur;
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        }
        if (d <= s0) {
            e->bend[0] = v0;
        } else if (v0 < cur && cur < 0x8000) {
            e->bend[0] -= s0;
        } else {
            e->bend[0] += s0;
        }
        cur = e->bend[1];
        if (cur > 0x8000) {
            d = v1 + (0x10000 - cur);
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        } else {
            d = v1 - cur;
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        }
        if (d <= s1) {
            e->bend[1] = v1;
        } else if (v1 < cur && cur < 0x8000) {
            e->bend[1] -= s1;
        } else {
            e->bend[1] += s1;
        }
        cur = e->bend[2];
        if (cur > 0x8000) {
            d = v2 + (0x10000 - cur);
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        } else {
            d = v2 - cur;
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        }
        if (d <= s2) {
            e->bend[2] = v2;
        } else if (v2 < cur && cur < 0x8000) {
            e->bend[2] -= s2;
        } else {
            e->bend[2] += s2;
        }
        cur = e->bend[3];
        if (cur > 0x8000) {
            d = v3 + (0x10000 - cur);
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        } else {
            d = v3 - cur;
            if (d >= 0x10000) {
                d = (u16) (0x10000 - d);
            }
        }
        if (d <= s3) {
            e->bend[3] = v3;
        } else if (v3 < cur && cur < 0x8000) {
            e->bend[3] -= s3;
        } else {
            e->bend[3] += s3;
        }
    } else {
        cur = e->bend[0];
        t0 = 0x10000 - v0;
        v0 = t0 - cur;
        if (v0 >= 0x8000) {
            v0 = (u16) (0x10000 - v0);
        }
        if (v0 <= s0) {
            e->bend[0] = (u16) t0;
        } else if (cur < t0 && cur > 0x8000) {
            e->bend[0] += s0;
        } else {
            e->bend[0] -= s0;
        }
        cur = e->bend[1];
        t1 = 0x10000 - v1;
        v1 = t1 - cur;
        if (v1 >= 0x8000) {
            v1 = (u16) (0x10000 - v1);
        }
        if (v1 <= s1) {
            e->bend[1] = (u16) t1;
        } else if (cur < t1 && cur > 0x8000) {
            e->bend[1] += s1;
        } else {
            e->bend[1] -= s1;
        }
        cur = e->bend[2];
        t2 = 0x10000 - v2;
        v2 = t2 - cur;
        if (v2 >= 0x8000) {
            v2 = (u16) (0x10000 - v2);
        }
        if (v2 <= s2) {
            e->bend[2] = (u16) t2;
        } else if (cur < t2 && cur > 0x8000) {
            e->bend[2] += s2;
        } else {
            e->bend[2] -= s2;
        }
        cur = e->bend[3];
        t3 = 0x10000 - v3;
        v3 = t3 - cur;
        if (v3 >= 0x8000) {
            v3 = (u16) (0x10000 - v3);
        }
        if (v3 <= s3) {
            e->bend[3] = (u16) t3;
        } else if (cur < t3 && cur > 0x8000) {
            e->bend[3] += s3;
        } else {
            e->bend[3] -= s3;
        }
    }
}

extern s8 gAreaObjCount;
s8 Em_NearestActionToBone(Em *em, s32 bone, f32 *out)
{
    f32 pos[4];
    f32 best = -1.0f;
    s32 i;
    u8 *a;
    EmAction **pp;
    f32 d;

    if (gAreaObjCount == 0) {
        return 0;
    }
    for (i = 0, pp = gAreaObjList; i < 32; i++, pp++) {
        a = (u8 *) *pp;
        if (a != NULL) {
            Em_BonePos(em, bone, pos);
            d = Vec_Distance(pos, (f32 *) (a + 0x30));
            if (d <= ((f32 *) em->sightTable)[6]) {
                if (best == -1.0f) {
                    best = d;
                    out[0] = *(f32 *) (a + 0x30);
                    out[1] = *(f32 *) (a + 0x34);
                    out[2] = *(f32 *) (a + 0x38);
                } else if (d < best) {
                    best = d;
                    out[0] = *(f32 *) (a + 0x30);
                    out[1] = *(f32 *) (a + 0x34);
                    out[2] = *(f32 *) (a + 0x38);
                }
            }
        }
    }
    return best == -1.0f ? 0 : 1;
}

/* If em may act (unk7F0 within its kind's limit, unk390 clear) and some player is active in its
 * area, finds the nearest area object (gAreaObjList) of em's area whose radius reaches em; with a clear
 * line to it (AreaHit_Raycast) its three ids go to em->actionGoal and it returns 1, else they are 0xFF.
 * n counts the players away, then flags a find (one register in the game). */
s32 Em_FindNearAreaObj(Em *em)
{
    EmObj *e;
    u8 n = 0;
    f32 v[4];
    f32 out[4];
    f32 best = -1.0f;
    f32 d;
    u8 k;
    u8 i;
    EmAction **w;
    u8 j;

    if (em->need1Gauge <= gEmRateTbl[em->type]->unk08 && em->stance == 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gPlayers[i].active == 0 || Obj_SameArea(em, &gPlayers[i]) == 0) {
                n++;
            }
        }
        if (n == gScene.playerCount) {
            n = 0;
        } else {
            for (j = 0, n = 0, w = gAreaObjList; j < 32; j++, w++) {
                e = (EmObj *) *w;
                if (e != NULL && e->area == em->area) {
                    d = Vec_Distance(&em->posX, e->pos);
                    if (d <= e->radius) {
                        n = 1;
                        if (best == -1.0f) {
                            best = d;
                            k = j;
                        } else if (!(best <= d)) {
                            best = d;
                            k = j;
                        }
                    }
                }
            }
        }
    }
    if (n == 0) {
        em->actionGoal[0] = em->actionGoal[2] = em->actionGoal[1] = 0xFF;
        return 0;
    }
    e = (EmObj *) gAreaObjList[k];
    Vec_Copy(v, &em->posX);
    v[1] += 100.0f;
    if (AreaHit_Raycast(v, (f32 *)e, out, em->collMask) == 0) {
        em->actionGoal[1] = e->goalArg;
        em->actionGoal[2] = e->goalNo;
        em->actionGoal[0] = e->kind;
        return 1;
    }
    em->actionGoal[0] = em->actionGoal[2] = em->actionGoal[1] = 0xFF;
    return 0;
}

EmAction *Em_FindGoalAction(Em *em)
{
    s8 i;
    EmAction *a;

    if (gAreaObjCount == 0) {
        return NULL;
    }
    for (i = 0; i < 32; i++) {
        a = gAreaObjList[i];
        if (a != NULL && em->actionGoal[1] == a->unk00[0x10] && em->actionGoal[2] == a->unk00[0x11]) {
            return a;
        }
    }
    return NULL;
}

void Em_StartGoalAction(Em *em, s32 n)
{
    if (em->remote == 0) {
        em->goalKind = 7;
        em->goalArg = (s16) em->actionGoal[1];
        em->goalNo = (s16) em->actionGoal[2];
        Em_ComputeGoal(em, em->goal);
    }
    Em_UseGoalAction(em);
    Em_Add7F0(em, n);
}

void Em_MoveRotated(Em *em, void *src)
{
    f32 v[3];
    f32 out[3];
    f32 m[16];

    v[0] = em->velX;
    v[1] = em->velY;
    v[2] = em->velZ;
    Mtx_FromAnglesYXZ(src, m);
    Vec_Rotate(out, v, m);
    em->posX += out[0];
    em->posY += out[1];
    em->posZ += out[2];
}

/* Moves the monster by its velocity (+0x3B8, turned by the angles ang), then adds its acceleration
 * (+0x3C4) to the velocity. */
void Em_MoveRotatedAccel(Em *em, void *ang)
{
    f32 v[4];
    f32 d[4];
    f32 m[16];

    Mtx_FromAnglesYXZ(ang, m);
    v[0] = *(f32 *) &em->velX;
    v[1] = em->velY;
    v[2] = em->velZ;
    Vec_Rotate(d, v, m);
    em->posX = em->posX + d[0];
    em->posY = em->posY + d[1];
    em->posZ = em->posZ + d[2];
    *(f32 *) &em->velX += *(f32 *) &em->accelX;
    em->velY += *(f32 *) &em->accelY;
    em->velZ += em->accelZ;
}

void Em_ChangeState(Em *em, s32 group, s32 state, s32 mode)
{
    switch (em->type) {
    case 0x1:
    case 0xB:
    case 0x25:
    case 0x29:
    case 0x2A:
    case 0x31:
        EmRathalos_ChangeState(em, group, state, mode);
        return;
    case 0x2:
    case 0x24:
    case 0x47:
        EmFatalis_ChangeState(em, group, state, mode);
        return;
    case 0x3:
    case 0x45:
        EmAnteka_ChangeState(em, group, state, mode);
        return;
    case 0x4:
    case 0x5:
    case 0x20:
        EmMosswine_ChangeState(em, group, state, mode);
        return;
    case 0x6:
    case 0x14:
    case 0x26:
    case 0x27:
        EmKutku_ChangeState(em, group, state, mode);
        return;
    case 0x7:
    case 0x32:
        EmLaoshan_ChangeState(em, group, state, mode);
        return;
    case 0x8:
    case 0x22:
        EmCephadrome_ChangeState(em, group, state, mode);
        return;
    case 0x9:
    case 0x17:
        EmMelynx_ChangeState(em, group, state, mode);
        return;
    case 0x39:
        EmShakalaka_ChangeState(em, group, state, mode);
        return;
    case 0xC:
    case 0x19:
    case 0x46:
        EmApceros_ChangeState(em, group, state, mode);
        return;
    case 0xD:
    case 0x10:
    case 0x1E:
    case 0x23:
        EmVelociprey_ChangeState(em, group, state, mode);
        return;
    case 0xE:
    case 0x1A:
    case 0x2B:
    case 0x2C:
        EmDiablos_ChangeState(em, group, state, mode);
        return;
    case 0xF:
    case 0x2D:
        EmKhezu_ChangeState(em, group, state, mode);
        return;
    case 0x11:
    case 0x16:
    case 0x2F:
        EmGravios_ChangeState(em, group, state, mode);
        return;
    case 0x15:
    case 0x2E:
        EmPlesioth_ChangeState(em, group, state, mode);
        return;
    case 0x1B:
    case 0x1C:
    case 0x1F:
        EmDrome_ChangeState(em, group, state, mode);
        return;
    case 0x13:
    case 0x18:
        EmVespoid_ChangeState(em, group, state, mode);
        return;
    case 0x0:
    default:
        Em_SetStateRand(em, group, state);
        return;
    case 0x21:
        EmKirin_ChangeState(em, group, state, mode);
        return;
    case 0x28:
        EmGaruga_ChangeState(em, group, state, mode);
        return;
    case 0x30:
    case 0x42:
    case 0x43:
    case 0x49:
        EmHermitaur_ChangeState(em, group, state, mode);
        return;
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x3D:
    case 0x3E:
        EmBlangonga_ChangeState(em, group, state, mode);
        return;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        EmKushala_ChangeState(em, group, state, mode);
        return;
    case 0x37:
        EmShenGaoren_ChangeState(em, group, state, mode);
        return;
    case 0x38:
        EmKind38_ChangeState(em, group, state, mode);
        return;
    case 0x3A:
    case 0x48:
        EmYamaTsukami_ChangeState(em, group, state, mode);
        return;
    case 0x3B:
        EmChameleos_ChangeState(em, group, state, mode);
        return;
    case 0x3F:
        EmRemobra_ChangeState(em, group, state, mode);
        return;
    case 0x44:
        EmBulldrome_ChangeState(em, group, state, mode);
        return;
    }
}

void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c)
{
    if (em->remote == 0) {
        Em_ComputeGoal(em, em->goal);
    } else {
        Em_ComputeGoal2(em, em->goal);
    }
    Em_ChangeState(em, a, b, c);
}

void Em_SetState(Em *em, s32 group, s32 state)
{
    em->speedBonus = 0;
    Player_ForceState(em, group, state);
    em->stateRequest = 1;
}

void Em_SetStateRand(Em *em, s32 group, s32 state)
{
    em->stateRand = Rand_Next(0);
    Em_SetState(em, group, state);
}

void Em_SetStateSync(Em *em, s32 group, s32 state, u8 mode)
{
    if (Game_IsOnline() == 1) {
        switch (mode) {
        case 0:
            Em_SetState(em, group, state);
            break;
        case 1:
            Em_SyncState(em, group, state, 0);
            break;
        case 2:
            Em_SyncState(em, group, state, 0);
            Em_SetState(em, group, state);
            break;
        case 3:
            Em_SyncState(em, group, state, 0);
            if (em->remote == 0) {
                Em_SetState(em, group, state);
            }
            break;
        case 4:
            Em_SyncState(em, group, state, 1);
            if (em->remote == 0) {
                Em_SetState(em, group, state);
            }
            break;
        }
    } else {
        em->stateRand = Rand_Next(0);
        Em_SetState(em, group, state);
    }
}

void Em_MotionDelta(Em *em, f32 *out)
{
    f32 a[4];
    f32 b[4];
    Model *model;
    f32 t;
    f32 step;

    step = em->motion[0].step;
    model = em->model;
    if (em->motion[0].blend != 0) {
        if (em->motion[0].blend < 0) {
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
        } else {
            Bone_GetAnimMoveFrom(em->motion[0].frame, em->motion[0].frame + step, a, model->partBones[0]->child);
            t = Math_Hermite(em->motion[0].blendT, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
            Bone_GetAnimMoveFrom(em->motion[0].blendFrame, em->motion[0].blendFrame + step, b, model->partBones2[0]->child);
            Vec_Blend(out, a, b, 1.0f - t, t);
        }
    } else {
        Bone_GetAnimMoveFrom(em->motion[0].frame - step, em->motion[0].frame, out, model->partBones[0]->child);
    }
    out[0] *= em->scaleX;
    out[1] *= em->scaleY;
    out[2] *= em->scaleZ;
}

//---
void Em_CheckRageByHp(Em *em, s16 n)
{
    s32 k;
    f32 pct;
    EmRageRow *t = gEmRageTbl[em->type];

    if (t == NULL) {
        return;
    }
    if (em->type == 0x34) {
        pct = em->need1Gauge * 100 / em->need1Max;
    } else if (em->type == 0x35) {
        if (EmBlangonga_PartFlagUnset(em, 6) == 0) {
            return;
        }
        pct = em->hp * 100 / em->hpMax;
    } else {
        pct = em->hp * 100 / em->hpMax;
    }
    if (pct == 100.0f) {
        k = 0;
    } else if (!(pct < 90.0f)) {
        k = 1;
    } else if (!(pct < 80.0f)) {
        k = 2;
    } else if (!(pct < 70.0f)) {
        k = 3;
    } else if (!(pct < 60.0f)) {
        k = 4;
    } else if (!(pct < 50.0f)) {
        k = 5;
    } else if (!(pct < 40.0f)) {
        k = 6;
    } else if (!(pct < 30.0f)) {
        k = 7;
    } else if (!(pct < 20.0f)) {
        k = 8;
    } else if (!(pct < 10.0f)) {
        k = 9;
    } else {
        k = 10;
    }
    if (em->type == 0x36 || em->type == 0x3C) {
        if (Em_PartBroken(em, 0) == 1) {
            n += n;
        }
    }
    if (em->type == 0x40 || em->type == 0x41) {
        if (Em_PartBroken(em, 0) == 1) {
            n += n;
        }
    }
    em->rageBuildup += (s16) ((f32) n * t->buildupByHp[k]);
    if (em->rageBuildup >= em->rageThreshold && em->ragePending == 0) {
        em->rageBuildup = em->rageThreshold;
        em->ragePending = 1;
    }
}

void Em_StartRage2(Em *em)
{
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if (rage != NULL) {
        em->ragePending = 0;
        em->rage = 1;
        em->rageTimer = rage->time;
        em->attackFactor = rage->attackFactor * Em_RageScale1((Em *) em);
        em->damageFactor = rage->damageFactor * Em_RageScale2((Em *) em);
        em->scriptPending = 1;
        em->triggers |= 8;
        em->triggers &= 0xFD;
    }
}

void Em_StartRage(Em *em)
{
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if (rage != NULL) {
        em->rage = 1;
        em->rageTimer = rage->time;
        em->attackFactor = rage->attackFactor * Em_RageScale1((Em *) em);
        em->damageFactor = rage->damageFactor * Em_RageScale2((Em *) em);
    }
}

void Em_StartPendingRage(Em *em)
{
    s16 t;
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if ((rage != NULL) && (em->versusGauge != 0)) {
        em->rage = 1;
        em->rageStarted = 1;
        if (em->remote == 0) {
            t = (em->versusGauge * 0x1E) + 0x1C2;
            em->rageTimer = t;
            em->timer = (s32) t;
        } else {
            em->rageTimer = (s16) em->timer;
        }
        em->attackFactor = rage->attackFactor * Em_RageScale1((Em *) em);
        em->damageFactor = rage->damageFactor * Em_RageScale2((Em *) em);
        em->versusGauge = 0;
    }
}

void Em_UpdateReactions(Em *em)
{
    s32 i;
    EmReactView *s = (EmReactView *) em;
    EmReaction *t;

    for (i = 0; i < 4; i++) {
        if (s->last[i] != s->cur[i]) {
            s->last[i] = s->cur[i];
            t = Em_GetReaction(em, 1, i);
            Em_AddNotice(em, t->unk0, t->unk4, i);
        }
    }
}

static s16 Em_NoticeTime(u8 type)
{
    s32 temp_v1;

    temp_v1 = type & 0xFF;
    switch (temp_v1) {
    case 16:
    case 35:
    case 13:
    case 30:
    case 27:
    case 28:
    case 31:
        return 0x384;
    case 40:
        return 0xA8C;
    default:
        return 0x708;
    }
}

/* Adds `add` to the player's anger count (Em +0x86C[player], kept below `max`); from 9000 on, with the
 * player in the monster's area, flags the player (+0x7E3) and sets its timer (+0x7E4). */
void Em_AddNotice(Em *em, s32 add, s32 max, u8 player)
{
    s32 i = player;
    Player *pl = &gPlayers[i];
    s32 *v;
    s32 n;
    s32 off;

    if (pl->active != 0 && pl->stateGroup != 3) {
        /* a byte offset of its own, added before the monster's address (`&em->noticeCount[i]` adds em
         * first) */
        off = i * 4;
        v = (s32 *) (off + (s32) em + 0x86C);
        n = *v + add;
        if (n < max) {
            *v = n;
        }
        if (*v >= 9000 && Obj_SameArea(em, pl) != 0) {
            em->noticed |= 1 << player;
            em->noticeTimer[i] = Em_NoticeTime(em->type);
        }
    }
}

/* Lowers the anger counts (+0x86C) each frame by the kind's rates (in the area or not), and calls the
 * players who hit (+0x533) or used a flagged action in the area to the monster's attention. */
void Em_DecayNotice(Em *em)
{
    s32 i;
    s32 *rate = gEmNoticeDecayRates[em->type];
    Player *p;

    if (em->combat == 1 && em->targetNo != -1) {
        p = &gPlayers[em->targetNo & 0xF];
        if (Obj_SameArea(em, p) != 0 && p->active != 0) {
            if (rate[0] < em->noticeCount[em->targetNo]) {
                em->noticeCount[em->targetNo] -= rate[0];
            } else {
                em->noticeCount[em->targetNo] = 0;
            }
        } else {
            if (rate[2] < em->noticeCount[em->targetNo]) {
                em->noticeCount[em->targetNo] -= rate[2];
            } else {
                em->noticeCount[em->targetNo] = 0;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        EmReaction *t;

        if (gPlayers[i].active == 0) {
            continue;
        }
        if (!(em->sensedPlayers & (1 << i)) || (em->noticeCount[i] >= 30000 && em->targetNo != i)) {
            if (Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].active != 0) {
                if (rate[1] < em->noticeCount[i]) {
                    em->noticeCount[i] -= rate[1];
                } else {
                    em->noticeCount[i] = 0;
                }
            } else {
                if (rate[3] < em->noticeCount[i]) {
                    em->noticeCount[i] -= rate[3];
                } else {
                    em->noticeCount[i] = 0;
                }
            }
        }
        if (gPlayers[i].carry & 0xF) {
            t = Em_GetReaction(em, 2, i);
            Em_AddNotice(em, t->unk0, t->unk4, i);
        }
        if (Player_TestFlag(&gPlayers[i], 0x10000) != 0 && Obj_SameArea(em, &gPlayers[i]) != 0) {
            t = Em_GetReaction(em, 8, i);
            Em_AddNotice(em, t->unk0, t->unk4, i);
            if (t->unk0 != 0) {
                em->noticed |= (u8) (1 << i);
                em->noticeTimer[i] = Em_NoticeTime(em->type);
            }
        }
    }
}

u8 Em_PlayerActionKind(u8 player)
{
    u8 r = 0;
    Player *p = &gPlayers[player];

    switch (p->action) {
    case 1:
        if (Player_TestFlag(p, 0x0201020B) != 0) {
            r = 1;
        }
        break;
    case 0:
    case 2:
    case 3:
        r = 2;
        if (Player_TestFlag(p, 0x02000200) != 0) {
            r = 4;
        } else if (Player_TestFlag(p, 0x1000B) != 0) {
            r = 3;
        }
        break;
    }
    return r;
}

/* Adds to the aggro value of player `player` (unk848, the per-kind tables gEmPlayerActionKinds)
 * by reaction kind 0-4 and the player's rank r (Em_PlayerActionKind), unless the player has skill
 * 0x13 (r 4 still counts). Every case is one EM_AGGRO_ADD, whose rank argument is copied to the
 * block-local s32 k (the table index): a first web per case, so the masked rank is coloured as
 * in the game (a1; with r itself as the index, a0 and idx's mask a1: 21 words). The tests are on
 * r; case 1's `r > 0` gives blez for the game's beqz (1 word). */
#define EM_AGGRO_ADD(cond, rank, row) {                   \
        s32 k = (rank);                                   \
        if (cond) {                                       \
            em->alertBuildup[pi] += base[idx];            \
            em->alertBuildup[pi] += tbl[row][k][idx];     \
        }                                                 \
    }

s32 Player_HasSkill(Player *p, u8 skill);
void Em_AddAlertBuildup(Em *em, u8 player, u8 idx, s8 kind)
{
    u8 r;
    Player *pl;
    s32 *base;
    s32 (**tbl)[4];
    s32 pi;

    pi = player;
    pl = &gPlayers[pi];
    tbl = gEmPlayerActionKinds[em->type];
    base = (s32 *) tbl[0];

    if (em->alertBuildup[pi] < gEmPartBuildupLimits[em->type][em->alertLimitRow] && Em_ScriptFlagsFrom(em, 0x10) == 0 && em->combat == 0) {
        r = Em_PlayerActionKind(player);
        if (Player_HasSkill(pl, 0x13) != 1 || r == 4) {
            switch (kind) {
            case 0:
                EM_AGGRO_ADD(1, r, 1);
                break;
            case 1:
                EM_AGGRO_ADD(r != 0, r, 2);
                break;
            case 2:
                EM_AGGRO_ADD(r > 1, r, 3);
                break;
            case 3:
                EM_AGGRO_ADD(r > 2, r, 4);
                break;
            case 4:
                EM_AGGRO_ADD(1, r, 1);
                break;
            }
        }
    }
}

#undef EM_AGGRO_ADD

extern f32 *gEmReactionDists[];
extern u16 *gEmReactionAngles[];
/* Loads the monster kind's reaction set k: four distance limits (+0x764, script command 0x83),
 * the four reaction ranges (+0x838) and the four half-angles (+0x860) from the per-kind tables;
 * +0x972 = k. The row pointers are loaded first and offset by k in bytes afterwards (as one
 * expression, MWCC reuses the address registers). */
void Em_LoadReactionSet(Em *em, u8 k)
{
    f32 *f;
    u16 *s;

    f = gEmReactionDists[em->type];
    s = gEmReactionAngles[em->type];
    f = (f32 *)((u32)f + k * 32);
    s = (u16 *)((u32)s + k * 8);
    em->reactionSet = k;
    em->reactDist[0] = *f++;
    em->reactDist[1] = *f++;
    em->nearRadius = *f++;
    *(f32 *) ((u8 *) em + 0x770) = *f++; /* limit 3: Em has the Player fields inBattle, prevInBattle here */
    em->reactRange[0] = *f++;
    em->sectorHalfAngle[0] = *s++;
    em->reactRange[1] = *f++;
    em->sectorHalfAngle[1] = *s++;
    em->reactRange[2] = *f++;
    em->sectorHalfAngle[2] = *s++;
    *(f32 *) ((u8 *) em + 0x844) = *f++; /* range 3: Em has the Player field aimPitch at +0x846 */
    em->sectorHalfAngle[3] = *s++;
}

/* the monster's position tables (Em_AreaEntryValue, Em_FindAreaEntry) */
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
AreaRecord *Stage_GetAreaRecord(u16 x);
void Em_PickRoutePoint(Em *em);
extern u8 gEmGoalPosSets[];
extern f32 *gEmGoalPoints[];

void Em_ComputeGoal(Em *em, f32 *target)
{
    s8 i;
    f32 *pt;
    EmRouteSet *set;
    EmRoutePt *rp;
    EmObj *o;
    u8 found;
    EmObj **w;
    f32 *pts;
    u16 k;
    u16 ang;
    f32 s;
    f32 out6[3];
    f32 d6[3];
    s32 rot6[3];
    s32 rot11[3];
    f32 d11[3];
    f32 out11[3];
    f32 m6[16];
    f32 m11[16];

    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    switch (em->goalKind) {
    case 1:
        switch (em->curGoalArg) {
        case 0:
        if (em->curGoalNo == -1) {
            em->targetNo = -1;
            em->target = NULL;
        } else {
        if (em->combat == 1 && (s8) em->scriptPlayer != -1) {
            em->goalNo = (s8) em->scriptPlayer & 0xF;
            em->curGoalNo = *(u16 *) &em->goalNo;
        }
        em->targetNo = *(u16 *) &em->goalNo;
        em->target = &gPlayers[em->targetNo];
            Em_GetTargetPos(em, em->targetNo, target);
        }
        break;
        }
        break;
    case 2:
        if (*(u16 *) &em->goalArg == 7) {
            pts = Em_AreaEntryValue(em, (struct EmIdEntry *)gEmGoalPosSets);
        } else {
            pts = Em_AreaEntryValue(em, em->tables[*(u16 *) &em->goalArg]);
        }
        switch (*(u16 *) &em->goalArg) {
        case 0:
        first:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = em->posY;
                target[2] = 5000.0f;
            } else {
                target[0] = pts[0];
                target[1] = pts[1];
                target[2] = pts[2];
            }
            break;
        case 1:
        case 7:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = 0.0f;
                target[2] = 5000.0f;
            } else {
                pt = pts + *(u16 *) &em->goalNo * 3;
                target[0] = pt[0];
                target[1] = pt[1];
                target[2] = pt[2];
            }
            break;
        case 2:
            em->curGoalNo = *(u16 *) &em->goalNo;
            if (pts == NULL) {
                pt = (f32 *)Stage_GetAreaRecord(em->area);
                switch (*(u16 *) &em->goalNo) {
                case 0:
                    target[0] = 0.2f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.2f * pt[5];
                    break;
                case 1:
                    target[0] = 0.2f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.8f * pt[5];
                    break;
                case 2:
                    target[0] = 0.8f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.8f * pt[5];
                    break;
                case 3:
                default:
                    target[0] = 0.8f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.2f * pt[5];
                    break;
                }
            } else {
                pt = pts + *(u16 *) &em->goalNo * 3;
                target[0] = pt[0];
                target[1] = pt[1];
                target[2] = pt[2];
            }
            break;
        case 3:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            pt = pts + (em->curGoalNo = *(u16 *) &em->goalNo) * 3;
            target[0] = pt[0];
            target[1] = pt[1];
            target[2] = pt[2];
            break;
        case 4:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            pt = pts + *(u16 *) &em->goalNo * 3;
            target[0] = pt[0];
            target[1] = pt[1];
            target[2] = pt[2];
            break;
        case 5:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            pt = pts + *(u16 *) &em->goalNo * 3;
            target[0] = pt[0];
            target[1] = pt[1];
            target[2] = pt[2];
            break;
        }
        break;
    case 3:
        em->nextArea = *(u16 *) &em->goalNo;
        Em_PickRoutePoint(em);
        em->goalArg = *(u16 *) &em->destArea;
        break;
    case 5:
        pt = gEmGoalPoints[em->type];
        target[0] = pt[em->curGoalNo * 3 + 0];
        target[1] = pt[em->curGoalNo * 3 + 1];
        target[2] = pt[em->curGoalNo * 3 + 2];
        break;
    case 6:
        switch (em->curGoalArg) {
        case 0:
            s = 1.0f;
            ang = 0;
            break;
        case 1:
            s = 1.0f;
            ang = 0xC000;
            break;
        case 2:
            s = 1.0f;
            ang = 0x4000;
            break;
        case 3:
            s = 1.0f;
            ang = 0x8000;
            break;
        case 8:
            s = 2.0f;
            ang = 0;
            break;
        case 9:
            s = 2.0f;
            ang = 0xC000;
            break;
        case 10:
            s = 2.0f;
            ang = 0x4000;
            break;
        case 11:
            s = 2.0f;
            ang = 0x8000;
            break;
        }
        d6[0] = 0.0f;
        d6[1] = 0.0f;
        d6[2] = 500.0f * s;
        rot6[0] = 0;
        rot6[1] = (u16) (em->rotY + ang);
        rot6[2] = 0;
        Mtx_FromAnglesXYZ(rot6, m6);
        Vec_Rotate(out6, d6, m6);
        target[0] = em->posX + out6[0];
        target[1] = em->posY + out6[1];
        target[2] = em->posZ + out6[2];
        break;
    case 7:
        found = 0;
        if (gAreaObjCount != 0) {
            for (i = 0, w = (EmObj **) gAreaObjList; i < 32; i++, w++) {
                o = *w;
                if (o != NULL && o->goalArg == em->curGoalArg && o->goalNo == em->curGoalNo) {
                    found = 1;
                    target[0] = o->pos[0];
                    target[1] = o->pos[1];
                    target[2] = o->pos[2];
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        pts = Em_AreaEntryValue(em, em->tables[0]);
        goto first;
    case 8:
        target[0] = em->objectPos[0];
        target[1] = em->objectPos[1];
        target[2] = em->objectPos[2];
        break;
    case 9:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        if (set == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (set->routes == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        rp = set->routes[em->curGoalArg].pts;
        if (rp == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        target[0] = rp[em->curGoalNo].pos[0];
        target[1] = rp[em->curGoalNo].pos[1];
        target[2] = rp[em->curGoalNo].pos[2];
        em->routeNo = em->curGoalArg;
        em->routePoint = em->curGoalNo;
        break;
    case 11:
        em->targetNo = em->curGoalArg;
        em->target = &gPlayers[em->targetNo];
        k = *(u16 *) ((u8 *) em->target + 0x662);
        pt = Em_AreaEntryValue(em, em->tables[0]) + k * 3;
        k = Math_AngleXZ(pt, &em->posX);
        rot11[0] = 0;
        rot11[1] = k;
        rot11[2] = 0;
        switch (em->curGoalNo) {
        case 0:
            d11[0] = 0.0f;
            d11[1] = 500.0f;
            d11[2] = 1000.0f;
            break;
        case 1:
            d11[0] = 0.0f;
            d11[1] = 400.0f;
            d11[2] = 800.0f;
            break;
        case 2:
            d11[0] = 0.0f;
            d11[1] = 400.0f;
            d11[2] = 1500.0f;
            break;
        case 3:
            d11[0] = 0.0f;
            d11[1] = 600.0f;
            d11[2] = 600.0f;
            break;
        }
        Mtx_FromAnglesYXZ(rot11, m11);
        Vec_Rotate(out11, d11, m11);
        target[0] = pt[0] + out11[0];
        target[1] = pt[1] + out11[1];
        target[2] = pt[2] + out11[2];
        break;
    case 13:
        switch (em->curGoalArg) {
        case 0:
        if (em->curGoalNo == -1) {
            em->targetNo = -1;
            em->target = NULL;
        } else {
        if (em->combat == 1 && (s8) em->scriptPlayer != -1) {
            em->goalNo = (s8) em->scriptPlayer % 20;
            em->curGoalNo = *(u16 *) &em->goalNo;
        }
        em->targetNo = *(u16 *) &em->goalNo;
        em->target = &gEms[em->targetNo];
            Em_GetTargetPos(em, em->targetNo, target);
        }
        break;
        }
        break;
    }
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
}

void Em_ComputeGoal2(Em *em, f32 *target)
{
    s8 i;
    f32 *pt;
    EmRouteSet *set;
    EmRoute *r;
    EmRoutePt *rp;
    EmObj *o;
    u8 found;
    EmObj **w;
    f32 *pts;
    u16 k;
    u16 ang;
    f32 s;
    f32 out6[3];
    f32 d6[3];
    s32 rot6[3];
    s32 rot11[3];
    f32 d11[3];
    f32 out11[3];
    f32 m6[16];
    f32 m11[16];

    em->goalKind = em->curGoalKind;
    em->goalArg = em->curGoalArg;
    em->goalNo = em->curGoalNo;
    switch (em->curGoalKind) {
    case 1:
        switch (em->curGoalArg) {
        case 0:
            if (em->curGoalNo == -1) {
                em->targetNo = -1;
                em->target = NULL;
            } else {
                em->targetNo = em->curGoalNo;
                em->target = &gPlayers[em->curGoalNo];
                Em_GetTargetPos(em, em->targetNo, target);
                em->scriptPlayer = em->curGoalNo;
            }
            break;
        }
        break;
    case 2:
        if (em->curGoalArg == 7) {
            pts = Em_AreaEntryValue(em, (struct EmIdEntry *)gEmGoalPosSets);
        } else {
            pts = Em_AreaEntryValue(em, em->tables[em->curGoalArg]);
        }
        switch (em->curGoalArg) {
        case 0:
        first:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = em->posY;
                target[2] = 5000.0f;
            } else {
                target[0] = pts[0];
                target[1] = pts[1];
                target[2] = pts[2];
            }
            break;
        case 1:
        case 7:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = 0.0f;
                target[2] = 5000.0f;
            } else {
                pt = pts + em->curGoalNo * 3;
                target[0] = pt[0];
                target[1] = pt[1];
                target[2] = pt[2];
            }
            break;
        case 2:
            if (pts == NULL) {
                pt = (f32 *)Stage_GetAreaRecord(em->area);
                switch (em->curGoalNo) {
                case 0:
                    target[0] = 0.2f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.2f * pt[5];
                    break;
                case 1:
                    target[0] = 0.2f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.8f * pt[5];
                    break;
                case 2:
                    target[0] = 0.8f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.8f * pt[5];
                    break;
                case 3:
                default:
                    target[0] = 0.8f * pt[4];
                    target[1] = 1000.0f + pt[6];
                    target[2] = 0.2f * pt[5];
                    break;
                }
            } else {
                pt = pts + em->curGoalNo * 3;
                target[0] = pt[0];
                target[1] = pt[1];
                target[2] = pt[2];
            }
            break;
        case 3:
        case 4:
        case 5:
            if (pts != NULL) {
                pt = pts + em->curGoalNo * 3;
                target[0] = pt[0];
                target[1] = pt[1];
                target[2] = pt[2];
            } else {
                goto fallback;
            }
            break;
        }
        break;
    case 3:
        em->destArea = em->curGoalArg;
        em->nextArea = em->curGoalNo;
        break;
    case 5:
        pt = gEmGoalPoints[em->type];
        target[0] = pt[em->curGoalNo * 3 + 0];
        target[1] = pt[em->curGoalNo * 3 + 1];
        target[2] = pt[em->curGoalNo * 3 + 2];
        break;
    case 6:
        switch (em->curGoalArg) {
        case 0:
            s = 1.0f;
            ang = 0;
            break;
        case 1:
            s = 1.0f;
            ang = 0xC000;
            break;
        case 2:
            s = 1.0f;
            ang = 0x4000;
            break;
        case 3:
            s = 1.0f;
            ang = 0x8000;
            break;
        case 8:
            s = 2.0f;
            ang = 0;
            break;
        case 9:
            s = 2.0f;
            ang = 0xC000;
            break;
        case 10:
            s = 2.0f;
            ang = 0x4000;
            break;
        case 11:
            s = 2.0f;
            ang = 0x8000;
            break;
        }
        d6[0] = 0.0f;
        d6[1] = 0.0f;
        d6[2] = 500.0f * s;
        rot6[0] = 0;
        rot6[1] = (u16) (em->rotY + ang);
        rot6[2] = 0;
        Mtx_FromAnglesXYZ(rot6, m6);
        Vec_Rotate(out6, d6, m6);
        target[0] = em->posX + out6[0];
        target[1] = em->posY + out6[1];
        target[2] = em->posZ + out6[2];
        break;
    case 7:
        found = 0;
        if (gAreaObjCount == 0) {
            goto fallback;
        }
        em->actionGoal[1] = em->curGoalArg;
        em->actionGoal[2] = em->curGoalNo;
        for (i = 0, w = (EmObj **) gAreaObjList; i < 32; i++, w++) {
            o = *w;
            if (o != NULL && o->goalArg == em->curGoalArg && o->goalNo == em->curGoalNo) {
                found = 1;
                target[0] = o->pos[0];
                target[1] = o->pos[1];
                target[2] = o->pos[2];
                break;
            }
        }
        if (found) {
            break;
        }
    fallback:
        pts = Em_AreaEntryValue(em, em->tables[0]);
        goto first;
    case 8:
        target[0] = em->objectPos[0];
        target[1] = em->objectPos[1];
        target[2] = em->objectPos[2];
        break;
    case 9:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        if (set == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        r = set->routes;
        if (r == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (em->curGoalArg >= set->n) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        rp = r[em->curGoalArg].pts;
        if (rp == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (em->curGoalNo >= r->n) {
            em->curGoalNo = r->n - 1;
        }
        target[0] = rp[em->curGoalNo].pos[0];
        target[1] = rp[em->curGoalNo].pos[1];
        target[2] = rp[em->curGoalNo].pos[2];
        em->routeNo = em->curGoalArg;
        em->routePoint = em->curGoalNo;
        break;
    case 11:
        em->targetNo = em->curGoalArg;
        em->target = &gPlayers[em->targetNo];
        k = *(u16 *) ((u8 *) em->target + 0x662);
        pt = Em_AreaEntryValue(em, em->tables[0]) + k * 3;
        k = Math_AngleXZ(pt, &em->posX);
        rot11[0] = 0;
        rot11[1] = k;
        rot11[2] = 0;
        switch (em->curGoalNo) {
        case 0:
            d11[0] = 0.0f;
            d11[1] = 500.0f;
            d11[2] = 1000.0f;
            break;
        case 1:
            d11[0] = 0.0f;
            d11[1] = 400.0f;
            d11[2] = 800.0f;
            break;
        case 2:
            d11[0] = 0.0f;
            d11[1] = 400.0f;
            d11[2] = 1500.0f;
            break;
        case 3:
            d11[0] = 0.0f;
            d11[1] = 600.0f;
            d11[2] = 600.0f;
            break;
        }
        Mtx_FromAnglesYXZ(rot11, m11);
        Vec_Rotate(out11, d11, m11);
        target[0] = pt[0] + out11[0];
        target[1] = pt[1] + out11[1];
        target[2] = pt[2] + out11[2];
        break;
    case 12:
        if (em->curGoalArg == 0) {
            em->trapTimer = em->curGoalNo * 4;
        }
        break;
    case 13:
        switch (em->curGoalArg) {
        case 0:
            if (em->curGoalNo == -1) {
                em->targetNo = -1;
                em->target = NULL;
            } else {
                em->targetNo = em->curGoalNo;
                em->target = &gEms[em->curGoalNo];
                Em_GetTargetPos(em, em->targetNo, target);
                em->scriptPlayer = em->curGoalNo;
            }
            break;
        }
        break;
    }
    em->goalKind = em->curGoalKind;
    em->goalArg = em->curGoalArg;
    em->goalNo = em->curGoalNo;
}

/* the players in sight (seen) become noticed; the others' notice timers count down */
u8 Em_UpdateNoticed(Em *em)
{
    s32 i;

    for (i = 0; i < gScene.playerCount; i++) {
        if (em->seen & (1 << i)) {
            em->noticed |= (u8) (1 << i);
            em->noticeTimer[i] = Em_NoticeTime(em->type);
        } else if (em->noticed & (1 << i)) {
            if (--em->noticeTimer[i] <= 0) {
                em->noticed &= (u8) ~(1 << i);
            }
        }
    }
    return em->noticed;
}

/* Each frame, for each player active in the monster's area: the distance (+0x818), the distance and
 * direction in the ground plane (+0x828, +0x858); within reach (the kind's radius) it marks the
 * player (+0x868), flags it (+0x7E3, with a timer) within the kind's limit for Em_AngleSector's class,
 * and runs the reaction of the first reaction range (+0x838; Em_GetReaction kinds 3 to 6) that holds
 * the player. flat: the monster's and the player's positions at height 0. */
void Em_SensePlayers(Em *em)
{
    EmSenseView *v = (EmSenseView *) em;
    f32 out[3];
    s32 i;
    f32 *lim;
    s32 bit;
    s8 k;
    f32 flat[2][3];
    f32 *to = flat[1];
    EmReaction *t;

    for (i = 0; i < 4; i++) {
        if (gPlayers[i].active != 0) {
            if (i < gScene.playerCount) {
                if (Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].stateGroup != 3) {
                    lim = gEmSenseLimits[em->type];
                    Em_GetTargetPos(em, i, out);
                    v->dist[i] = Vec_Distance(&em->posX, out);
                    Vec_Set(flat[0], em->posX, 0.0f, em->posZ);
                    Vec_Set(to, out[0], 0.0f, out[2]);
                    v->distXZ[i] = Vec_Distance(flat[0], to);
                    v->dir[i] = Math_AngleXZ(flat[0], to);
                    if (v->dist[i] <= ((f32 *) em->sightTable)[7]) {
                        bit = 1 << i;
                        em->sensedPlayers |= bit;
                        k = Em_AngleSector(em, i);
                        if (v->dist[i] <= lim[k]) {
                            em->noticed |= bit;
                            v->unk7E4[i] = Em_NoticeTime(em->type);
                        }
                        if (!(v->range[0] < v->dist[i]) && !(v->range[0] < 0.0f)) {
                            t = Em_GetReaction(em, 3, i);
                            Em_AddNotice(em, t->unk0, t->unk4, i);
                            Em_AddAlertBuildup(em, i, 0, k);
                        } else if (!(v->range[1] < v->dist[i]) && !(v->range[1] < 0.0f)) {
                            t = Em_GetReaction(em, 4, i);
                            Em_AddNotice(em, t->unk0, t->unk4, i);
                            Em_AddAlertBuildup(em, i, 1, k);
                        } else if (!(v->range[2] < v->dist[i]) && !(v->range[2] < 0.0f)) {
                            t = Em_GetReaction(em, 5, i);
                            Em_AddNotice(em, t->unk0, t->unk4, i);
                            Em_AddAlertBuildup(em, i, 2, k);
                        } else if (!(v->range[3] < v->dist[i]) && !(v->range[3] < 0.0f)) {
                            t = Em_GetReaction(em, 6, i);
                            Em_AddNotice(em, t->unk0, t->unk4, i);
                            Em_AddAlertBuildup(em, i, 3, k);
                        }
                    } else {
                        em->sensedPlayers &= ~(1 << i);
                        v->unk848[i] = 0;
                    }
                } else {
                    em->sensedPlayers &= ~(1 << i);
                    v->unk848[i] = 0;
                }
            } else {
                em->sensedPlayers &= ~(1 << i);
                v->unk848[i] = 0;
            }
        }
    }
}

/* A player whose anger count (+0x848) reached the kind limit, when its bit of +0x868 is set: sets its bit in
 * +0x869 and +0x86B bit 4, clears the count. */
void Em_CheckPartBuildup(Em *em)
{
    s32 i;
    s32 lim = gEmPartBuildupLimits[em->type][em->alertLimitRow];

    for (i = 0; i < 4; i++) {
        if (Em_ScriptFlagsFrom(em, 0x10) == 0 && em->combat != 1 && em->noReact == 0 && em->alertBuildup[i] >= lim
            && (em->sensedPlayers & (1 << i))) {
            em->scriptPending = 1;
            em->triggers |= 0x10;
            em->alertedPlayers |= 1 << i;
            em->alertBuildup[i] = 0;
        }
    }
}

/*
 * Monster helpers: the per-kind setup dispatch, angles and target
 * positions, the +0x967 priority requests, counters decreased by per-kind
 * rates, checks against sub_main's object tables, and pending placements.
 * See include/em.h.
 */



extern EmReaction *gEmReactions[];
extern EmRate *gEmDecay7ECRates[];
extern EmRate *D_008BC7D0[];
extern u8 gEmCarryReact[];
extern s16 *gEmRequestIds[];
extern EmObj *gEmSenseObjs[32];
extern s8 gEmSenseObjCount;

u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
s32 Game_IsOnline(void);
u16 Rand_Next(s32 stream);
u8 Quest_FindSpecialMonster(u8 type);
s32 Player_HasSkill(Player *p, u8 skill);
s32 LightBowgun_HasOption10(Player *player);
s32 Obj_SameArea(void *obj, Player *b);
void QSync_SendEm(Em *em, s32 a, s32 b); /* unit's own declaration: the definition (gm_sub qnet_sync.c) takes u8 kind, u8 arg; u8 here changes Em_SyncState */
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
void Em_AddNotice(Em *em, s32 a, s32 b, u8 player);
void Em_Reduce7F0(Em *em);
void Em_Reduce7EC(Em *em);
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ApplyAreaValues(Em *em);
void EmDrome_Setup(Em *em);
void EmGaruga_Setup(Em *em);
void EmHermitaur_Setup(Em *em);
void EmGravios_Setup(Em *em);
void EmKutku_Setup(Em *em);
void EmPlesioth_Setup(Em *em);
void EmDiablos_Setup(Em *em);
void EmKhezu_Setup(Em *em);
void EmLaoshan_Setup(Em *em);
void EmCephadrome_Setup(Em *em);
void EmRathalos_Setup(Em *em);
void EmBlangonga_Setup(Em *em);
void EmShenGaoren_Setup(Em *em);
void EmKushala_Setup(Em *em);
void EmYamaTsukami_Setup(Em *em);
void EmChameleos_Setup(Em *em);
void EmBulldrome_Setup(Em *em);

/* Asks for action `a` with priority `prio` unless a higher one is pending. */
void Em_SetRequest(Em *em, u8 a, u8 prio)
{
    if (em->requestPrio <= prio) {
        em->requestPrio = prio;
        em->requested = 1;
        em->requestId = a;
        em->scriptRestart = 1;
    }
}

/* Does +0x78F have any bit at or above the lowest bit of `mask`? */
s32 Em_ScriptFlagsFrom(Em *em, u8 mask)
{
    s32 i;
    u8 m;

    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            break;
        }
    }
    if (i > 7) {
        return 0;
    }
    m = 0;
    for (; i < 8; i++) {
        m |= 1 << i;
    }
    return (em->scriptFlags & m) != 0;
}

void Em_TypeSetup(Em *em)
{
    Em_ApplyAreaValues(em);
    em->loopPoint = -1;
    em->loopCount = -1;
    switch (em->type) {
    case 1:
    case 0x2A:
    case 0x25:
    case 0xB:
    case 0x29:
    case 0x31:
        EmRathalos_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 7:
    case 0x32:
        EmLaoshan_Setup(em);
        break;
    case 8:
    case 0x22:
        EmCephadrome_Setup(em);
        break;
    case 0xE:
    case 0x2B:
    case 0x1A:
    case 0x2C:
        EmDiablos_Setup(em);
        break;
    case 0xF:
    case 0x2D:
        EmKhezu_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 0x11:
    case 0x2F:
    case 0x16:
        EmGravios_Setup(em);
        break;
    case 6:
    case 0x26:
    case 0x14:
    case 0x27:
        EmKutku_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 0x15:
    case 0x2E:
        EmPlesioth_Setup(em);
        break;
    case 0x1B:
    case 0x1C:
    case 0x1F:
        EmDrome_Setup(em);
        break;
    case 0x28:
        EmGaruga_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 0x30:
        Em_Reduce7F0(em);
    case 0x43:
        EmHermitaur_Setup(em);
        break;
    case 0x33:
    case 0x34:
    case 0x35:
        EmBlangonga_Setup(em);
        break;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        EmKushala_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 0x37:
        EmShenGaoren_Setup(em);
        break;
    case 0x3A:
    case 0x48:
        EmYamaTsukami_Setup(em);
        break;
    case 0x3B:
        EmChameleos_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case 0x44:
        EmBulldrome_Setup(em);
        break;
    }
}

/* The heading from `from` to `to` in the XZ plane (0x10000 is a full turn). */
u16 Math_AngleXZ(f32 *from, f32 *to)
{
    return Math_Heading(from[0], from[2], to[0], to[2]) - 0x4000;
}

void Em_GetPitchToTarget(Em *em, s32 *out, f32 a, f32 b)
{
    f32 target[4];
    f32 d;

    if (em->targetNo != -1) {
        Em_GetTargetPos(em, em->targetNo, target);
        d = Vec_DistanceXZ(&em->posX, target);
        *out = (u16)(0x10000 - Math_Heading(Math_Absf(d + b), em->posY, 0.0f, target[1] + a));
    }
}

void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b)
{
    f32 d = Vec_DistanceXZ(&em->posX, em->goal);

    *out = (u16)(0x10000 - Math_Heading(Math_Absf(d + b), em->posY,
                                       0.0f, em->goal[1] + a));
}

/* The position of target `idx`: a monster when +0x997 is 13, else a player. */
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out)
{
    Em *target;
    Player *player;

    if (em->curGoalKind == 13) {
        if (idx == -1) {
            return 0;
        }
        target = &gEms[idx];
        out[0] = target->posX;
        out[1] = target->posY;
        out[2] = target->posZ;
    } else if (Game_IsOnline() == 1) {
        if (idx == -1) {
            return 0;
        }
        player = &gPlayers[idx];
        if (em->large != 0) {
            out[0] = player->targetPos[0];
            out[1] = player->targetPos[1];
            out[2] = player->targetPos[2];
        } else {
            out[0] = player->pos[0];
            out[1] = player->pos[1];
            out[2] = player->pos[2];
        }
    } else {
        if (idx == -1) {
            return 0;
        }
        player = &gPlayers[idx];
        out[0] = player->pos[0];
        out[1] = player->pos[1];
        out[2] = player->pos[2];
    }
    return 1;
}

void Em_PickRequest(Em *em)
{
    s16 *ids = gEmRequestIds[em->type];

    if (em->requestPrio >= 0x80) {
        return;
    }
    if (Em_ScriptFlagsFrom(em, 0x80)) {
        return;
    }
    if (ids != NULL && em->combat == 1) {
        while (*ids != -1) {
            if (*ids >= 10000) {
                if (*ids - 10000 == em->area) {
                    return;
                }
            } else if (Area_DayId(*ids) == Area_DayId(em->area)) {
                return;
            }
            ids++;
        }
    }
    if (em->lowHpNeed != 0) {
        em->requestPrio = 0x80;
        Em_SetRequest(em, gEmKindInfo[em->type].lowHpRequest, 0x80);
        return;
    }
    if (em->requestPrio < 0x70) {
        if (em->need4 != 0) {
            em->need4 = 0;
            em->requestPrio = 0x70;
            Em_SetRequest(em, gEmKindInfo[em->type].need4Request, 0x70);
            return;
        }
        if (em->requestPrio < 0x60) {
            if (em->need1 != 0) {
                em->requestPrio = 0x60;
                Em_SetRequest(em, gEmKindInfo[em->type].need1Request, 0x60);
                return;
            }
            if (em->requestPrio < 0x50) {
                if (em->need2 != 0) {
                    em->requestPrio = 0x50;
                    Em_SetRequest(em, gEmKindInfo[em->type].need2Request, 0x50);
                    return;
                }
                if (em->requestPrio < 0x40 && em->need3 != 0 && em->combat == 0) {
                    em->requestPrio = 0x40;
                    Em_SetRequest(em, gEmKindInfo[em->type].need3Request, 0x40);
                }
            }
        }
    }
}

void Em_CheckLowHp(Em *em, f32 ratio)
{
    s16 *ids = gEmRequestIds[em->type];

    if ((u8)Quest_FindSpecialMonster(em->type) != 0xFF) {
        em->lowHpNeed = 0;
        return;
    }
    if (gEmKindInfo[em->type].lowHpRequest == 0) {
        em->lowHpNeed = 0;
        return;
    }
    if (em->stateGroup >= 5) {
        em->lowHpNeed = 0;
        return;
    }
    if (em->hp >= (s16)(em->hpMax * ratio)) {
        return;
    }
    if (em->requested == 1 && em->requestId == gEmKindInfo[em->type].lowHpRequest) {
        return;
    }
    if (Em_ScriptFlagsFrom(em, 0x80)) {
        return;
    }
    if (ids != NULL && em->combat == 1) {
        while (*ids != -1) {
            if (*ids >= 10000) {
                if (*ids - 10000 == em->area) {
                    return;
                }
            } else if (Area_DayId(*ids) == Area_DayId(em->area)) {
                return;
            }
            ids++;
        }
    }
    em->lowHpNeed = 1;
}

void Em_CheckCarryRequest(Em *em)
{
    u8 a;
    s8 i;
    s32 ok;

    if (gEmCarryReact[em->type] == 0) {
        return;
    }
    a = gEmKindInfo[em->type].need4Request;
    if (a == 0) {
        return;
    }
    if (em->requested == 1 && em->requestId == a) {
        for (i = 0; i < gScene.playerCount; i++) {
            ok = !(gPlayers[i].carry & 0xF);
            if (!ok) {
                return;
            }
        }
        em->requested = 0;
        em->requestId = 0;
        em->requestPrio = 0;
    } else {
        for (i = 0; i < gScene.playerCount; i++) {
            ok = !(gPlayers[i].carry & 0xF);
            if (!ok) {
                em->need4 = 1;
                return;
            }
        }
    }
}

void Em_Decay7F0(Em *em)
{
    EmRate *rate = gEmRateTbl[em->type];
    s32 n;

    if (em->combat == 0) {
        n = rate->unk0C;
    } else {
        n = rate->unk10;
    }
    em->need1Gauge -= n;
    if (em->need1Gauge < 0) {
        em->need1Gauge = 0;
    }
    if (gEmKindInfo[em->type].need1Request != 0) {
        switch (em->need1State) {
        case 0:
            if (em->need1Gauge == 0) {
                em->need1State = 1;
                em->need1 = 1;
            }
            break;
        }
    }
}

void Em_Decay7F0Only(Em *em)
{
    EmRate *rate = gEmRateTbl[em->type];
    s32 n;

    if (em->combat == 0) {
        n = rate->unk0C;
    } else {
        n = rate->unk10;
    }
    em->need1Gauge -= n;
    if (em->need1Gauge < 0) {
        em->need1Gauge = 0;
    }
}

void Em_Decay7EC(Em *em)
{
    EmRate *rate = gEmDecay7ECRates[em->type];
    s32 n;

    if (em->combat == 0) {
        n = rate->unk0C;
    } else {
        n = rate->unk10;
    }
    em->need2Gauge -= n;
    if (em->need2Gauge < 0) {
        em->need2Gauge = 0;
    }
    if (gEmKindInfo[em->type].need2Request != 0) {
        switch (em->need2State) {
        case 0:
            if (em->need2Gauge == 0) {
                em->need2 = 1;
                em->need2State = 1;
            }
            break;
        }
    }
}

void Em_Decay7F4(Em *em)
{
    EmRate *rate = D_008BC7D0[em->type];
    s32 n;

    if (em->combat == 0) {
        n = rate->unk08;
    } else {
        n = rate->unk0C;
    }
    em->need3Gauge -= n;
    if (em->need3Gauge < 0) {
        em->need3Gauge = 0;
    }
    if (gEmKindInfo[em->type].need3Request != 0) {
        switch (em->need3State) {
        case 0:
            if (em->need3Gauge == 0) {
                em->need3State = 1;
                em->need3 = 1;
            }
            break;
        }
    }
}

void Em_Reduce7F0(Em *em)
{
    if (em->need1Gauge > gEmRateTbl[em->type]->unk14) {
        em->need1Gauge -= gEmRateTbl[em->type]->unk14;
    } else {
        em->need1Gauge = 0;
    }
}

void Em_Reduce7EC(Em *em)
{
    if (em->need2Gauge > gEmDecay7ECRates[em->type]->unk14) {
        em->need2Gauge -= gEmDecay7ECRates[em->type]->unk14;
    } else {
        em->need2Gauge = 0;
    }
}

s8 Em_AngleSector(Em *em, s32 idx)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if ((u16)(em->playerAngle[idx] + em->sectorHalfAngle[i] - em->rotY) <= em->sectorHalfAngle[i] * 2) {
            return i;
        }
    }
    return i;
}

void Em_SyncState(Em *em, s32 kind, s32 b, s32 c)
{
    if (em->remote != 0) {
        return;
    }
    if (em->large != 0) {
        em->syncKind = kind;
        em->syncArg = b;
        em->syncGoalKind = em->curGoalKind;
        em->syncGoalArg = em->curGoalArg;
        em->syncGoalNo = em->curGoalNo;
        em->syncRand = Rand_Next(0);
        if ((u16) kind == 4 || (u16) kind == 5) {
            QSync_SendEm(em, 3, c);
        } else {
            QSync_SendEm(em, 1, c);
        }
    } else {
        em->stateRand = Rand_Next(0);
        Em_SetState(em, kind, b);
    }
}

s32 Em_InBlockArea1(Em *em)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && obj->area == em->area && (obj->kind & 1)
            && Vec_Distance(obj->pos, &em->posX) <= obj->radius) {
            return 1;
        }
    }
    return 0;
}

s32 Em_InBlockArea2(Em *em)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && obj->area == em->area && (obj->kind & 2)
            && (em->stance == 0 || em->stance == 1) && em->requestPrio < 0x80
            && Vec_Distance(obj->pos, &em->posX) <= obj->radius) {
            return 1;
        }
    }
    return 0;
}

u8 EmObj_AlertMonsters(EmObj *obj)
{
    f32 pos[4];
    s32 i;
    s32 hit = 0;
    Em *em = gEms;
    Player *player;
    EmReaction *r;

    for (i = 0; i < 20; i++, em++) {
        if (em->active == 0) {
            continue;
        }
        player = &gPlayers[obj->goalArg];
        if (Obj_SameArea(em, player) && player->stateGroup != 3) {
            em->noticed |= 1 << obj->goalArg;
            em->noticeTimer[obj->goalArg] = Em_NoticeTime(em->type);
            r = Em_GetReaction(em, 7, obj->goalArg);
            Em_AddNotice(em, r->unk0, r->unk4, obj->goalArg);
            Em_GetTargetPos(em, obj->goalArg, pos);
            if (Vec_Distance(obj->pos, pos) <= obj->radius) {
                hit = 1;
            }
        }
    }
    return hit;
}

EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player)
{
    Player *p = &gPlayers[player];
    EmReaction *table = gEmReactions[em->type];

    switch (kind) {
    case 1:
        if (Player_HasSkill(p, 0x14) == 1) {
            kind += 8;
        } else if (LightBowgun_HasOption10(p) == 1) {
            kind = 0x12;
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
        if (Player_HasSkill(p, 0x14) == 1) {
            kind += 7;
        } else if (Player_HasSkill(p, 0x13) == 1 && (u8)Em_PlayerActionKind(player) == 0) {
            kind += 0xB;
        }
        break;
    }
    return &table[kind];
}

/* Applies the pending placements at +0x8B4. */
void Em_ApplyPendingSpawn(Em *em)
{
    s32 i;
    u8 a;
    u8 b;

    for (i = 0; i < 2; i++) {
        if (em->spawns[i].pending != 0) {
            em->spawns[i].pending = 0;
            if (em->taskPhase > 0 && !((u32)(em->stateGroup - 4) <= 1) && em->spawns[i].arg == 0) {
                em->posX = em->spawns[i].pos[0];
                em->posY = em->spawns[i].pos[1];
                em->posZ = em->spawns[i].pos[2];
                em->rotX = em->spawns[i].rot[0];
                em->rotY = em->spawns[i].rot[1];
                em->rotZ = em->spawns[i].rot[2];
                a = em->spawns[i].stateGroup;
                b = em->spawns[i].state;
                em->curGoalKind = em->spawns[i].goalKind;
                em->curGoalArg = em->spawns[i].goalArg;
                em->curGoalNo = em->spawns[i].goalNo;
                em->area = em->spawns[i].area;
                em->timer = em->spawns[i].timer;
                em->stateRand = em->spawns[i].rand;
                em->stateGroup = a;
                em->state = b;
                Em_ChangeStateRouted(em, a, b, 0);
            }
        }
    }
}


/* Online (more than one player): which player Em +0x7E2 names (likely the one that runs the monster).
 * While Em +0x817 is set and that player has left (gScene.inQuest is 0xFF), the next player still in (+0x224 == 1)
 * becomes the target when that is the local player (QSync_SendEm kind 4). Otherwise, when a player in
 * the area other than the local one is in and the local player has not taken it (+0x682/+0x684),
 * picks the next such player after the local one, sets +0x817/+0x971 and calls Em_ApplyPendingSpawn.
 * Registers: the found flag is kept in j (a later web of j, numbered after the loop walkers),
 * which gives the saved registers (mask s0, em s1, found s2, i s3, &gScene + i s4, player s5).
 * The second part's count is an s32 m = (s32) gScene.playerCount (the loop's own load, no mask),
 * and the counting loops start `for (k = 0, cnt = 0, bits = 0; ...)`.
 * The area compare goes through pl = &gPlayers[i] (permuter, round 18; with gPlayers[i] directly
 * MWCC loads em->area first whichever way the == is written).
 * The remote branch counts into a and copies the count to m, the second part copies its count to n:
 * so the last loop's a, its bound n and the bound m are later webs, numbered after the hoisted
 * mask extension and constant 1, and coloured before them (round 24). */
void Em_MigrateHost(Em *em)
{
    s8 mask = 0;
    s8 i;
    s8 k;
    s8 j;
    s8 bits;
    s8 cnt;
    s8 a;
    s32 n;
    s32 m;
    Player *pl;

    if (em->large == 0 || gScene.playerCount == 1) {
        return;
    }
    if (em->hostTimer != 0) {
        em->hostTimer--;
    }
    if (em->remote > 0) {
        if (gScene.inQuest[em->hostNo] == 0xFF) {
            n = gScene.playerCount;
            for (i = 0, a = 0, bits = 0; i < n; i++) {
                if (gScene.inQuest[i] == 1) {
                    a++;
                    bits |= 1 << i;
                }
            }
            m = a;
            if (m < 2) {
                em->remote = 0;
                return;
            }
            j = 0;
            for (k = 0; k < n; k++) {
                j %= n;
                if (bits & (1 << j)) {
                    break;
                }
                j++;
            }
            if (j == gScene.localPlayer) {
                em->remote = 0;
                em->hostTimer = 0x3C;
                em->hostNo = j;
                QSync_SendEm(em, 4, 0);
            }
        }
        return;
    }
    j = 0;
    for (i = 0; i < gScene.playerCount; i++) {
        if (gPlayers[i].areaMoveState != 0 && i == gScene.localPlayer) {
            pl = &gPlayers[i];
            if (pl->nextArea == em->area && gScene.inQuest[i] == 1) {
                j = 1;
            }
        } else if (Obj_SameArea(em, &gPlayers[i]) && gPlayers[i].active) {
            if (i == gScene.localPlayer && gScene.inQuest[i] == 1) {
                return;
            }
            mask |= 1 << i;
        }
    }
    if (j != 0 && mask == 0) {
        return;
    }
    if (!mask) {
        return;
    }
    m = (s32) gScene.playerCount;
    for (k = 0, cnt = 0, bits = 0; k < m; k++) {
        if (gScene.inQuest[k] == 1) {
            cnt++;
            bits |= 1 << k;
        }
    }
    n = cnt;
    if (n < 2) {
        em->remote = 0;
        return;
    }
    j = gScene.localPlayer + 1;
    for (k = 0; k < m; k++) {
        j %= m;
        if (bits & (1 << j)) {
            break;
        }
        j++;
    }
    for (a = 0; a < n; a++) {
        if ((mask & (1 << j)) && gScene.inQuest[j] == 1) {
            em->hostTimer = 0x3C;
            em->hostNo = j;
            QSync_SendEm(em, 4, 0);
            em->hostChanged = 1;
            em->remote = 1;
            Em_ApplyPendingSpawn(em);
            return;
        }
        j++;
        for (k = 0; k < m; k++) {
            j %= m;
            if (bits & (1 << j)) {
                break;
            }
            j++;
        }
    }
}

u8 Em_SenseObjects(Em *em, EmObj *self)
{
    s8 i;
    EmObj *obj;

    if (gEmSenseObjCount == 0) {
        return 0;
    }
    if (em->senseDelay != 0) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        obj = gEmSenseObjs[i];
        if (obj == NULL || obj == self || obj->area != em->area) {
            continue;
        }
        switch (obj->kind) {
        case 0:
            if (em->type != 0x10 && em->type != 0x23 && em->type != 0x1B) {
                continue;
            }
            break;
        case 1:
            if (em->type != 9) {
                continue;
            }
            break;
        case 2:
            if (em->type != 0xC && em->type != 0x46 && em->type != 0x19) {
                continue;
            }
            break;
        case 3:
            if (em->type != 0x33 && em->type != 0x3D) {
                continue;
            }
            break;
        case 4:
            if (em->type != 0x33 && em->type != 0x3D) {
                continue;
            }
            break;
        }
        if (!(obj->radius < Vec_Distance(obj->pos, &em->posX))) {
            Vec_Set(em->objectPos, obj->pos[0], obj->pos[1], obj->pos[2]);
            return 1;
        }
    }
    return 0;
}

/* unk_701810's .data (0x8BAC10-0x8BCC98) */
extern u32 gEmKindTables_68[];
extern u32 D_008F5920[];
extern u32 D_008F5930[];
extern u32 D_008F5940[];
extern u32 D_008F5990[];
extern u32 D_008F59E0[];
extern u32 D_008F5A30[];
f32 gEmReactionDists_0[16] = {
#include "game/008BAC10.inc"
};
f32 gEmReactionDists_2[16] = {
#include "game/008BAC50.inc"
};
f32 gEmReactionDists_3[8] = {
#include "game/008BAC90.inc"
};
f32 gEmReactionDists_4[8] = {
#include "game/008BACB0.inc"
};
f32 gEmReactionDists_8[8] = {
#include "game/008BACD0.inc"
};
f32 gEmReactionDists_9[8] = {
#include "game/008BACF0.inc"
};
f32 gEmReactionDists_12[8] = {
#include "game/008BAD10.inc"
};
f32 gEmReactionDists_14[16] = {
#include "game/008BAD30.inc"
};
f32 gEmReactionDists_15[16] = {
#include "game/008BAD70.inc"
};
f32 gEmReactionDists_13[8] = {
#include "game/008BADB0.inc"
};
f32 gEmReactionDists_17[16] = {
#include "game/008BADD0.inc"
};
f32 gEmReactionDists_21[24] = {
#include "game/008BAE10.inc"
};
f32 gEmReactionDists_40[16] = {
#include "game/008BAE70.inc"
};
f32 gEmReactionDists_52[16] = {
#include "game/008BAEB0.inc"
};
f32 gEmReactionDists_53[16] = {
#include "game/008BAEF0.inc"
};
f32 gEmReactionDists_54[16] = {
#include "game/008BAF30.inc"
};
f32 gEmReactionDists_57[8] = {
#include "game/008BAF70.inc"
};
f32 gEmReactionDists_66[8] = {
#include "game/008BAF90.inc"
};
f32 gEmReactionDists_68[8] = {
#include "game/008BAFB0.inc"
};
f32 gEmReactionDists_71[16] = {
#include "game/008BAFD0.inc"
};
f32 *gEmReactionDists[76] = {
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_2, gEmReactionDists_3,
    gEmReactionDists_4, gEmReactionDists_4, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_8, gEmReactionDists_9, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_12, gEmReactionDists_13, gEmReactionDists_14, gEmReactionDists_15,
    gEmReactionDists_13, gEmReactionDists_17, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_21, gEmReactionDists_0, gEmReactionDists_9,
    gEmReactionDists_0, gEmReactionDists_12, gEmReactionDists_14, gEmReactionDists_13,
    gEmReactionDists_13, gEmReactionDists_0, gEmReactionDists_13, gEmReactionDists_13,
    gEmReactionDists_0, gEmReactionDists_3, gEmReactionDists_8, gEmReactionDists_13,
    gEmReactionDists_2, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_40, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_14,
    gEmReactionDists_14, gEmReactionDists_15, gEmReactionDists_21, gEmReactionDists_17,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_52, gEmReactionDists_53, gEmReactionDists_54, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_57, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_52, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_66, gEmReactionDists_0,
    gEmReactionDists_68, gEmReactionDists_3, gEmReactionDists_12, gEmReactionDists_71,
    gEmReactionDists_0, gEmReactionDists_66, NULL, NULL,
};
u16 gEmReactionAngles_0[8] = {
#include "game/008BB140.inc"
};
u16 *gEmReactionAngles[76] = {
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, NULL, NULL,
};
u32 gEmKindTables[76] = {
#include "game/008BB280.inc"
};
extern f32 D_00911410[];
extern f32 D_00913B90[];
extern f32 D_00914140[];
extern f32 D_00919780[];
extern f32 D_0091C440[];
f32 *gEmGoalPoints[76] = {
    NULL, NULL, NULL, NULL,
    D_00911410, D_00911410, NULL, NULL,
    NULL, NULL, D_00913B90, NULL,
    D_00914140, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, D_00919780, NULL, NULL,
    NULL, D_00914140, NULL, NULL,
    NULL, D_0091C440, NULL, NULL,
    D_00911410, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, D_00919780, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    D_00911410, NULL, D_00914140, NULL,
    NULL, NULL, NULL, NULL,
};
f32 gEmSenseLimits_1[4] = {
#include "game/008BB4E0.inc"
};
f32 gEmSenseLimits_4[4] = {
#include "game/008BB4F0.inc"
};
f32 gEmSenseLimits_13[4] = {
#include "game/008BB500.inc"
};
f32 gEmSenseLimits_40[4] = {
#include "game/008BB510.inc"
};
f32 *gEmSenseLimits[76] = {
    NULL, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_4, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, (f32 *)D_008F5920, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_13, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_13, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_13,
    gEmSenseLimits_13, (f32 *)D_008F5920, gEmSenseLimits_13, gEmSenseLimits_13,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_13,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_40, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, (f32 *)D_008F5920, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, NULL, NULL,
};
s32 gEmPartBuildupLimits_1[4] = {
#include "game/008BB650.inc"
};
s32 *gEmPartBuildupLimits[76] = {
    NULL, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, NULL, NULL,
};
u32 D_008BB790[4] = {
#include "game/008BB790.inc"
};
u32 D_008BB7A0[20] = {
#include "game/008BB7A0.inc"
};
u32 D_008BB7F0[20] = {
#include "game/008BB7F0.inc"
};
u32 D_008BB840[20] = {
#include "game/008BB840.inc"
};
u32 D_008BB890[20] = {
#include "game/008BB890.inc"
};
u32 D_008BB8E0[8] = {
    (u32)D_008BB790, (u32)D_008BB7A0, (u32)D_008BB7F0, (u32)D_008BB840,
    (u32)D_008BB890, 0, 0, 0,
};
u32 D_008BB900[8] = {
    (u32)D_008F5930, (u32)D_008F5940, (u32)D_008F5990, (u32)D_008F59E0,
    (u32)D_008F5A30, 0, 0, 0,
};
u32 D_008BB920[4] = {
#include "game/008BB920.inc"
};
u32 D_008BB930[20] = {
#include "game/008BB930.inc"
};
u32 D_008BB980[20] = {
#include "game/008BB980.inc"
};
u32 D_008BB9D0[20] = {
#include "game/008BB9D0.inc"
};
u32 D_008BBA20[20] = {
#include "game/008BBA20.inc"
};
u32 D_008BBA70[8] = {
    (u32)D_008BB920, (u32)D_008BB930, (u32)D_008BB980, (u32)D_008BB9D0,
    (u32)D_008BBA20, 0, 0, 0,
};
s32 (**gEmPlayerActionKinds[76])[4] = {
    NULL, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB900, (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BBA70, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB900,
    (void *)D_008BB900, (void *)D_008BB900, (void *)D_008BB900, (void *)D_008BB900,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB900,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB900, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, (void *)D_008BB900, (void *)D_008BB8E0,
    (void *)D_008BB8E0, (void *)D_008BB8E0, NULL, NULL,
};
EmReaction gEmReactions_1[20] = {
#include "game/008BBBC0.inc"
};
EmReaction gEmReactions_2[20] = {
#include "game/008BBC60.inc"
};
EmReaction gEmReactions_3[20] = {
#include "game/008BBD00.inc"
};
EmReaction gEmReactions_9[20] = {
#include "game/008BBDA0.inc"
};
EmReaction gEmReactions_12[20] = {
#include "game/008BBE40.inc"
};
EmReaction gEmReactions_13[20] = {
#include "game/008BBEE0.inc"
};
EmReaction gEmReactions_22[20] = {
#include "game/008BBF80.inc"
};
EmReaction gEmReactions_27[20] = {
#include "game/008BC020.inc"
};
EmReaction gEmReactions_29[20] = {
#include "game/008BC0C0.inc"
};
EmReaction gEmReactions_66[20] = {
#include "game/008BC160.inc"
};
EmReaction *gEmReactions[76] = {
    NULL, gEmReactions_1, gEmReactions_2, gEmReactions_3,
    gEmReactions_3, gEmReactions_3, gEmReactions_1, gEmReactions_2,
    gEmReactions_1, gEmReactions_9, gEmReactions_1, gEmReactions_1,
    gEmReactions_12, gEmReactions_13, gEmReactions_1, gEmReactions_1,
    gEmReactions_13, gEmReactions_1, gEmReactions_1, gEmReactions_3,
    gEmReactions_1, gEmReactions_1, gEmReactions_22, gEmReactions_9,
    gEmReactions_3, gEmReactions_12, gEmReactions_1, gEmReactions_27,
    gEmReactions_27, gEmReactions_29, gEmReactions_13, gEmReactions_27,
    gEmReactions_3, gEmReactions_1, gEmReactions_1, gEmReactions_13,
    gEmReactions_2, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_22,
    gEmReactions_1, gEmReactions_9, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_66, gEmReactions_1,
    gEmReactions_3, gEmReactions_3, gEmReactions_12, gEmReactions_2,
    gEmReactions_1, gEmReactions_66, NULL, NULL,
};
s32 gEmNoticeDecayRates_1[4] = {
#include "game/008BC330.inc"
};
s32 gEmNoticeDecayRates_27[4] = {
#include "game/008BC340.inc"
};
s32 *gEmNoticeDecayRates[76] = {
    NULL, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_27,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, NULL, NULL,
};
EmRate gEmRateTbl_1 = {
#include "game/008BC480.inc"
};
EmRate gEmRateTbl_8 = {
#include "game/008BC4A0.inc"
};
EmRate gEmRateTbl_6 = {
#include "game/008BC4C0.inc"
};
EmRate gEmRateTbl_48 = {
#include "game/008BC4E0.inc"
};
EmRate gEmRateTbl_52 = {
#include "game/008BC500.inc"
};
EmRate *gEmRateTbl[76] = {
    NULL, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_1,
    &gEmRateTbl_8, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_6, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_6,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_48, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_52, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_48,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, NULL, NULL,
};
EmRate gEmDecay7ECRates_1 = {
#include "game/008BC650.inc"
};
EmRate gEmDecay7ECRates_15 = {
#include "game/008BC670.inc"
};
EmRate *gEmDecay7ECRates[76] = {
    NULL, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_15,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_15, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, NULL, NULL,
};
u32 gEmRateStatus_0[4] = {
#include "game/008BC7C0.inc"
};
/* The third per-kind rate table (D_008BC7D0, 74 entries) runs straight into the
 * 16-byte status parameter records that unk_7077A0's tables point to (D_008BC8F8,
 * D_008BC908, ...: aliases), so it is one object. */
typedef struct EmStatusRow {
    EmStatusParam param;
    s16 unkA;
    s32 unkC;
} EmStatusRow;
typedef struct EmRateStatusTbl {
    EmRate *rate[74];
    EmStatusRow status[58];
} EmRateStatusTbl;
EmRateStatusTbl gEmRateStatus = {
#include "game/008BC7D0.inc"
};
