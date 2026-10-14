/*
 * Game overlay code at 0x6FF630-0x701810, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "capsule.h"
/* prototypes of callees (round 13) */
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Quest_GetTime(s32 which);
#pragma optimization_level 4

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
typedef struct EmSightRange {
    f32 maxXZ;
    f32 minXZ;
    f32 above;
    f32 below;
    u16 half;
} EmSightRange;
u16 Mtx_Heading(f32 *m);
EmBone *Em_Bone(void *obj, s16 bone);
s32 AreaGrid_TestSegment(Em *em, f32 *a, f32 *b, f32 *out, u16 mask);
extern s16 AreaObj_K17_RectX[];
extern s16 AreaObj_K17_RectY[];
extern s16 AreaObj_K17_RectW[];
extern s16 AreaObj_K17_RectH[];
extern f32 AreaObj_K17_Spread1[];
extern s16 D_008F58C0[];
extern s16 D_008F58F0[];
void Bgm_PlayEvent(s32 se);
extern s16 AreaObj_K17_AngleTbl[];
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
extern EmMotionRow *gEmMotionRows[];
extern EmMotionRow *gEmMotionAltRows[];
extern EmMotionRow *gEmMotionTransRows[];
extern f32 *gEmSenseLimits[];
s8 Em_AngleSector(Em *em, s32 idx);
void Em_AddAlertBuildup(Em *em, u8 player, u8 idx, s8 kind);
extern s32 *gEmNoticeDecayRates[];
s32 EmBlangonga_PartFlagUnset(Em *em, u16 part);
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
void Em_CheckFlash(Em *em, u8 player, EmSight *s);
s32 Em_SightBlocked(Em *em, void **seg);
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
void Eff37_Create(f32 *pos, u8 k, u8 c);
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
void Em_Add7F0(Em *em, s32 n);
void Em_UseGoalAction(Em *em);
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s32 Sprite_Draw(Sprite *s);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetState63(s32 a);
void Tex_Bind(s32 i);
void DrawSlot_Free(s32 i);
extern u8 AreaObj_K19_PointsA2[];
extern u8 AreaObj_K19_PointsA3[];
extern u8 AreaObj_K19_PointsA6[];
extern u8 AreaObj_K19_PointsA7[];
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
typedef struct EmBendView {
    u8 pad[0x63C];
    u32 bend[4];
} EmBendView;
extern EmBendRow *gEmBendOrigins[];
/* end of prototypes */

void AreaObj_K17_Create(u8 a)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 0x17;
        obj->update = AreaObj_K17_Update;
        obj->type = a;
    }
}

void AreaObj_K17_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K17_Setup(o);
        break;
    case 1:
        AreaObj_K17_Run(o);
        break;
    case 2:
        AreaObj_K17_End(o);
        break;
    case 3:
        AreaObj_K17_Free(o);
        break;
    }
}

void AreaObj_K17_Setup(Obj *o)
{

    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->keep = 1;
    o->timer = 0;
    o->var3 = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = AreaObj_K17_Draw;
    } else {
        Obj_Free(o);
        return;
    }
    AreaObj_K17_Run(o);
}

void AreaObj_K17_Run(Obj *o)
{
    o->var3++;
    if (o->var3 >= 300) {
        o->state++;
        return;
    }
    switch (o->type) {
    case 4:
        if (o->var0 == 0) {
            if (++o->timer > 100) {
                o->timer = 100;
                o->var0++;
            }
        }
        break;
    case 7:
        if (++o->timer >= 100) {
            o->timer = 100;
            o->var0++;
        }
        break;
    case 6:
        if (++o->timer >= 0x71) {
            o->timer = 0x71;
            o->var0++;
        }
        break;
    }
    OT_AddByDepth(gDrawListText0b, o->ds, 1, 1);
}

void AreaObj_K17_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void AreaObj_K17_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K17_DrawTexRect(s16 *r, s32 angle)
{
    Gfx_SetState63(0U);
    Gfx_SetBlend(4U, 5U);
    Tex_Prepare(1, 0x154);
    Tex_Bind(0x154);
    TexRect_DrawRotated((struct TexRect *) r, angle);
}

/* Draws a gauge (p: x, y, w, h, colour) as two rotated texture rects, the second moved by a table
 * lookup (AreaObj_K17_AngleTbl, interpolated in 5-degree steps of the angle). */
void AreaObj_K17_DrawPart(s16 *p, u32 angle)
{
    Sprite r;
    u16 a;
    f32 f;
    u8 i;
    s32 d;

    Gfx_SetState63(0);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x154);
    Tex_Bind(0x154);
    r.color = *(s32 *) (p + 4);
    r.w = p[3] * 12 / 17;
    r.h = p[2];
    r.x = p[0] + (r.h - r.w) / 2;
    r.y = p[1] + (r.w - r.h) / 2;
    a = angle;
    r.u0 = 0xA0;
    r.v0 = 0x70;
    r.u1 = r.u0 + 0x5F;
    r.v1 = r.v0 + 0x8F;
    TexRect_DrawRotated((struct TexRect *) &r, (u16) (a + 0x4000));
    f = 10.0f + (360.0f * angle) / 65536.0f;
    if (!(f < 360.0f)) {
        f -= 360.0f;
    }
    i = f / 5.0f;
    switch (i) {
    case 0:
    case 1:
    case 2:
    case 3:
        d = (s16) (AreaObj_K17_AngleTbl[i] + ((f - 5.0f * i) * (AreaObj_K17_AngleTbl[i + 1] - AreaObj_K17_AngleTbl[i])) / 5.0f);
        break;
    case 4:
        d = AreaObj_K17_AngleTbl[i];
        break;
    default:
        d = AreaObj_K17_AngleTbl[2];
        break;
    }
    r.w = p[3] * 5 / 17;
    r.h = p[2];
    r.x = p[0] + (r.h - r.w) / 2;
    r.y = (p[1] + p[3]) - (r.w + r.h) / 2;
    r.x = r.x + d;
    r.y -= 6;
    r.u0 = 0xA0;
    r.v0 = 0x70;
    r.u1 = r.u0 + 0x27;
    r.v1 = r.v0 + 0x8F;
    TexRect_DrawRotated((struct TexRect *) &r, (u16) (a + 0x7FFF + 0x4001));
}

void AreaObj_K17_DrawSprite(Sprite *r)
{
    r->u0 = 0;
    r->v0 = 0;
    r->u1 = r->u0 + 0xFF;
    r->v1 = r->v0 + 0xFF;
    Gfx_SetState63(1U);
    Gfx_SetBlend(4U, 5U);
    Tex_Prepare(1, 0x146);
    Tex_Bind(0x146);
    Sprite_Draw(r);
}

/* Draw callback of the kind-0x17 objects (AreaObj_K17_Create; slot->owner is the Obj): by the type (+3: 4,
 * 7 or 6) a full-screen sprite (AreaObj_K17_DrawSprite) that fades in over 15 frames in phase 0 (+6), then
 * from phase 2 a rect scaled per phase at a per-type table position, shaken by a frame table (pos[1]),
 * drawn at the angle pos[0] (degrees) by AreaObj_K17_DrawTexRect or AreaObj_K17_DrawPart. Phase 1 lasts 60 frames (+8),
 * then picks the angle at random and plays sound 0x2A-0x2C. */
void AreaObj_K17_Draw(DrawSlot *slot)
{
    Obj *o = slot->owner;
    Sprite r;
    f32 scale;
    f32 alpha;
    u8 k;
    u8 type;
    u8 fr;

    r.x = 0;
    r.y = 0;
    type = o->type;
    switch (type) {
    case 4:
        k = 0;
        break;
    case 7:
        k = 1;
        break;
    case 6:
        k = 2;
        break;
    }
    switch (type) {
    case 4:
    case 7:
    case 6:
        r.w = 0x280;
        r.h = 0x1C0;
        switch (o->var1) {
        case 0:
            scale = 1.0f;
            alpha = o->timer / 15.0f;
            break;
        case 1:
        case 2:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        case 3:
            scale = 0.95f;
            alpha = 1.0f;
            break;
        case 4:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        }
        r.x = 0;
        r.y = 0;
        r.w = 640.0f * scale;
        r.h = 448.0f * scale;
        r.x -= (s16) ((r.w - 0x280) / 2);
        r.y -= (s16) ((r.h - 0x1C0) / 2);
        r.color = ((u8) (255.0f * alpha) << 24) | 0xFFFFFF;
        AreaObj_K17_DrawSprite(&r);
        switch (o->var1) {
        case 0:
        case 1:
            break;
        case 2:
            scale = 1.15f;
            alpha = 0.5f;
            break;
        case 3:
            scale = 0.9f;
            alpha = 1.0f;
            break;
        case 4:
            scale = 1.0f;
            alpha = 1.0f;
            break;
        }
        switch (o->var1) {
        case 0:
        case 1:
            break;
        case 2:
        case 3:
        case 4:
            r.w = scale * AreaObj_K17_RectW[k];
            r.h = scale * AreaObj_K17_RectH[k];
            r.x = AreaObj_K17_RectX[k] - r.w / 2;
            r.y = AreaObj_K17_RectY[k] - r.h / 2;
            fr = o->pos[1];
            if (fr >= 8) {
                fr = 0;
            }
            r.x += D_008F58C0[fr + k * 8];
            r.y += D_008F58F0[fr + k * 8];
            r.color = ((u8) (255.0f * alpha) << 24) | 0xFFFFFF;
            switch (o->type) {
            case 4:
            case 6:
                r.u0 = 0;
                r.v0 = 0;
                r.u1 = r.u0 + 0xFF;
                r.v1 = r.v0 + 0x6F;
                break;
            case 7:
                r.u0 = 0;
                r.v0 = 0x70;
                r.u1 = r.u0 + 0x9F;
                r.v1 = r.v0 + 0x8F;
                break;
            }
            switch (o->type) {
            case 4:
                AreaObj_K17_DrawTexRect(&r.x, (u16) (s32) (0.5f + (65536.0f * o->pos[0]) / 360.0f));
                break;
            case 7:
                AreaObj_K17_DrawTexRect(&r.x, (u16) (s32) (0.5f + (65536.0f * o->pos[0]) / 360.0f));
                break;
            case 6:
                AreaObj_K17_DrawPart(&r.x, (u16) (s32) (0.5f + (65536.0f * o->pos[0]) / 360.0f));
                break;
            }
            break;
        }
        switch (o->var1) {
        case 0:
            if (o->timer >= 15) {
                o->var1++;
            }
            break;
        case 1:
            if (o->timer >= 60) {
                o->var1++;
                o->pos[0] = AreaObj_K17_Spread1[k] * (-1.0f + (2.0f * Rand_Next(1)) / 65536.0f);
                o->pos[1] = (u16) Rand_Next(1) & 8;
                switch (o->type) {
                case 4:
                    Bgm_PlayEvent(0x2A);
                    break;
                case 7:
                    Bgm_PlayEvent(0x2C);
                    break;
                case 6:
                    Bgm_PlayEvent(0x2B);
                    break;
                }
            }
            break;
        case 2:
        case 3:
            o->var1++;
            break;
        }
    }
}

void AreaObj_K19_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x19;
        obj->update = AreaObj_K19_Update;
        *(u16 *) &obj->var3 = gScene.area;
    }
}

void AreaObj_K19_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K19_Setup(o);
        break;
    case 1:
        AreaObj_K19_Run(o);
        break;
    case 2:
        AreaObj_K19_End(o);
        break;
    case 3:
        AreaObj_K19_Free(o);
        break;
    }
}

void AreaObj_K19_Setup(Obj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var0 = 0;
    o->timer = 0;
    o->ds = NULL;
}

/* Every 300 frames (the counter +8), unless the quest time says otherwise, creates a kind-0x29 stage
 * object and its effect at one of the points of the area +0xA (AreaObj_K19_GetPoints, AreaObj_K19_PointCount), picked by
 * the quest time and seed. */
void AreaObj_K19_Run(Obj *o)
{
    u8 *p;
    s32 t;
    u16 k;
    s16 n;

    t = Quest_GetTime(0) - Quest_GetTime(1);
    k = t / 300;
    k += Quest_GetSeedByte(0);
    if (--o->timer <= 0) {
        if ((t & 0x40) != 0x40) {
            if ((t & 0x30) == 0) {
                if (gScene.eventRunning == 0) {
                    p = (u8 *) AreaObj_K19_GetPoints(o->var3);
                    n = AreaObj_K19_PointCount(o->var3);
                    if (p == NULL || n == 0) {
                        return;
                    }
                    p += (k % n) * 12;
                    StageObj_K29_CreateAt((f32 *) p, 0xF, 0x96, 0.7f);
                    Eff37_Create((f32 *) p, 1, 0x82);
                    o->timer = 300;
                }
            } else {
                o->timer = 0;
            }
        } else {
            o->timer = 0;
        }
    }
}

void AreaObj_K19_End(Obj *o)
{
    if (o->ds != NULL) {
        DrawSlot_Free(o->slot);
    }
    o->state++;
    o->visible = 0;
}

void AreaObj_K19_Free(Obj *o)
{
    Obj_Free(o);
}

s32 AreaObj_K19_GetPoints(u16 arg0)
{
    s32 temp_v1;

    temp_v1 = arg0 & 0xFFFF;
    switch (temp_v1) {
    case 0xA2:
        return (s32) AreaObj_K19_PointsA2;
    case 0xA3:
        return (s32) AreaObj_K19_PointsA3;
    case 0xA6:
        return (s32) AreaObj_K19_PointsA6;
    case 0xA7:
        return (s32) AreaObj_K19_PointsA7;
    default:
        return 0;
    }
}

s16 AreaObj_K19_PointCount(u16 arg0)
{
    s32 temp_v1;

    temp_v1 = arg0 & 0xFFFF;
    switch (temp_v1) {
    case 0xA2:
        return 6;
    case 0xA3:
        return 3;
    case 0xA6:
        return 2;
    case 0xA7:
        return 5;
    default:
        return 0;
    }
}

/* Which players the monster sees (each frame, after Em_SensePlayers; the type picks the eye bone):
 * a player is seen when in the monster area, within the height and ground-plane ranges of the row
 * at Em +0x638 (a zero range is no limit), within its half angle of the head direction, with no
 * obstacle (AreaGrid_TestSegment) or stage object (Em_SightBlocked) in the way. Sets the player bits at +0x7E0
 * (running reaction kind 0 for a newly seen one unless +0x7DC) and returns the farthest player
 * seen, or -1; with +0x7DF set, sees nobody. */
s8 Em_SeePlayers(Em *em)
{
    EmSenseView *v;
    EmSight sight;
    f32 pos[3];
    f32 hit[3];
    f32 eye[3];
    f32 pt[3];
    EmSightRange *r;
    u16 half;
    s32 i;
    s32 ang;
    s32 sel = -1;
    s32 t;
    f32 best;
    f32 maxXZ;
    Player *pl;
    f32 minXZ;
    f32 above;
    f32 below;
    f32 y;
    f32 dy;
    f32 d;
    f32 dxz;
    f32 *e;
    s32 h2;
    EmReaction *rx;

    if (em->blind != 0) {
        em->seen = 0;
        return (u8) sel;
    }
    em->targetDist = 0.0f;
    best = 0.0f;
    pl = gPlayers;
    switch (em->type) {
    case 1:
    case 0x2A:
    case 0x25:
    case 0xB:
    case 0x29:
    case 0x31:
    case 6:
    case 0x26:
    case 8:
    case 0xE:
    case 0x2B:
    case 0xF:
    case 0x2D:
    case 0x11:
    case 0x2F:
    case 0x14:
    case 0x27:
    case 0x15:
    case 0x2E:
    case 0x16:
    case 0x1A:
    case 0x2C:
    case 0x22:
    case 0x28:
        Em_BonePos(em, 0x22, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x22)) + 0x4000);
        break;
    case 2:
    case 0x24:
    case 0x47:
        Em_BonePos(em, 0x33, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x33)) + 0x4000);
        break;
    case 7:
    case 0x32:
        Em_BonePos(em, 0x23, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x23)) + 0x4000);
        break;
    case 9:
    case 0x17:
    case 0x39:
        pos[0] = em->posX;
        pos[1] = 100.0f + em->posY;
        pos[2] = em->posZ;
        ang = em->rotY;
        break;
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x3D:
    case 0x3E:
        Em_BonePos(em, 0x12, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x12)) + 0x4000);
        break;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        Em_BonePos(em, 0x25, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x25)) + 0x4000);
        break;
    case 0x3B:
        Em_BonePos(em, 0x25, pos);
        ang = (u16) (Mtx_Heading((f32 *)Em_Bone(em, 0x25)) + 0x4000);
        break;
    default:
        pos[0] = em->posX;
        pos[1] = 100.0f + em->posY;
        pos[2] = em->posZ;
        ang = em->rotY;
        break;
    }
    v = (EmSenseView *) em;
    r = *(EmSightRange **) &em->sightTable;
    maxXZ = r->maxXZ;
    y = em->posY;
    minXZ = r->minXZ;
    above = r->above;
    below = r->below;
    half = r->half;
    sight.dir = ang;
    h2 = half * 2;
    sight.half = half;
    sight.pos = pos;
    for (i = 0; i < 4; i++, pl++) {
        if (pl->stateGroup != 3 && Obj_SameArea(em, pl) && pl->active) {
            Em_GetTargetPos(em, i, pt);
            Em_CheckFlash(em, i, &sight);
            dy = pt[1] - y;
            if ((above == 0.0f || dy <= 0.0f || dy <= above)
                && (below == 0.0f || !(dy < 0.0f) || -dy <= below)) {
                dxz = v->distXZ[i];
                if ((maxXZ == 0.0f || dxz <= maxXZ)
                    && (minXZ == 0.0f || !(dxz < minXZ))
                    && (d = v->dist[i], (u16) (v->dir[i] + half - ang) <= h2)) {
                    if (pl->action == 1) {
                        Vec_Set(eye, pt[0], 50.0f + pt[1], pt[2]);
                    } else {
                        Vec_Set(eye, pt[0], 170.0f + pt[1], pt[2]);
                    }
                    sight.unk04 = (s32) (e = eye);
                    if (em->type != 0x38) {
                        if (gScene.unk20A != 0) {
                            t = AreaGrid_TestSegment(em, pos, e, hit, 0x4100);
                        } else {
                            t = AreaGrid_TestSegment(em, pos, e, hit, 0x100);
                        }
                        if (t == 1) {
                            goto out;
                        }
                    }
                    if ((u8) Em_SightBlocked(em, (void **) &sight) != 1) {
                        if (!(em->seen & (1 << i)) && em->combat == 0) {
                            rx = Em_GetReaction(em, 0, i);
                            Em_AddNotice(em, rx->unk0, rx->unk4, i);
                        }
                        em->seen |= 1 << i;
                        if (best == 0.0f) {
                            best = d;
                            sel = i;
                        } else if (!(d <= best)) {
                            best = d;
                            sel = i;
                        }
                        continue;
                    }
                }
            }
        }
out:
        em->seen &= ~(1 << i);
    }
    return (u8) sel;
}

extern s8 gFlashObjCount;
/* Sets Em +0x810 when one of the stage objects of 0x5D8B70 in the monster area is within reach of s->pos
 * and inside the angle s->dir +- s->half (not for the kinds listed, nor at +0x390 == 4 for some). */
void Em_CheckFlash(Em *em, u8 player, EmSight *s)
{
    s32 t;
    s32 i;
    EmObj **pp;
    EmObj *e;
    u16 a;

    if (*(u16 *) &em->noHitTimer > 0 || *(s8 *) &em->guardTimer > 0 || em->remote != 0) {
        return;
    }
    t = em->type;
    if (t == 2 || t == 0x24 || t == 5 || t == 0x32 || t == 7 || t == 8
        || t == 0xF || t == 0x27 || t == 0x14 || t == 0x15 || t == 0x2D
        || t == 0x2E || t == 0x18 || t == 0x1D || t == 0x21 || t == 0x22
        || t == 0x30 || t == 0x37 || t == 0x3A || t == 0x47 || t == 0x48
        || t == 0x49 || t == 0x42 || t == 0x43 || t == 0x44) {
        return;
    }
    if (t == 0xE || t == 0x2B || t == 0x1A || t == 0x2C) {
        if (em->stance == 4) {
            return;
        }
    }
    if (gFlashObjCount == 0) {
        return;
    }
    for (i = 0, pp = gFlashObjs; i < 32; i++, pp++) {
        e = *pp;
        if (e != NULL && e->area == em->area && Vec_Distance(e->pos, s->pos) <= e->radius) {
            a = Math_AngleXZ(s->pos, e->pos);
            if ((u16) (a + s->half - s->dir) <= s->half * 2) {
                em->flashed = 1;
            }
        }
    }
}

s32 Em_SightBlocked(Em *em, void **seg)
{
    f32 m[16];
    s32 i;
    u8 *o;
    u8 **pp;

    for (i = 0, pp = (u8 **) gSightBlockObjs; i < 32; i++, pp++) {
        o = *pp;
        if (o != NULL && *(u16 *) (o + 0x18) == em->area && (o[0x12] & 1)) {
            Segment_Init(seg[1], seg[0], (struct Segment *) m);
            if (Capsule_Classify((struct Segment *) m, (f32 *) o, *(f32 *) (o + 0xC)) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

void Em_SetRow638(Em *em, u8 idx)
{
    em->sightTable = gEmRows638[em->type] + idx * 0x20;
}

/* Starts row `motion` of the kind's motion table on every layer it names (EmMotionRow.layers), from
 * `frame` (the row's when 0), blending over blend / 2 frames (the row's, or Em +0x418, when 0). A layer
 * whose current motion's row names a transition row for this motion takes that row's frame and blend.
 * With Em +0x652 set, the row's `remap` row of the alternate table is used once. Em +0x418 is the row's
 * unk7 afterwards. The blend if/else and the (s8) (s32) / (s16) (s32) casts give the game's
 * sign-extensions; the layer's motion-id offset is i * 200 (a third induction variable). */
void Em_SetMotion(Em *em, s32 motion, s32 blend, s32 frame)
{
    EmMotionRow *tbl = gEmMotionRows[em->type];
    s32 i;
    u16 id;
    u16 cur;
    s32 k;
    EmMotionRow *sub;
    u8 s;
    u16 id2;

    Mtx_FromAnglesYXZ(&em->rotX, em->rotMtx[0]);
    if (em->motionRemap) {
        em->motionRemap = 0;
        if (tbl[motion].remap) {
            motion = tbl[motion].remap;
            tbl = gEmMotionAltRows[em->type];
        }
    }
    for (i = 0; i < em->motionLayerCount; i++) {
        switch (tbl[motion].layers) {
        case -1:
            break;
        case 0:
        case 1:
        case 2:
            if (i != tbl[motion].layers) {
                continue;
            }
            break;
        case 0x10:
            if (i == 2) {
                continue;
            }
            break;
        case 0x11:
            if (i == 1) {
                continue;
            }
            break;
        case 0x12:
            if (i == 0) {
                continue;
            }
            break;
        }
        if (frame == 0) {
            frame = tbl[motion].frame;
        }
        id = tbl[motion].motion + 1000 + i * 200;
        if (blend == 0) {
            if (em->rowBlend != 0) {
                if (tbl[motion].blend != 0) {
                    blend = (s8) (s32) tbl[motion].blend;
                } else {
                    blend = (s16) (s32) em->rowBlend;
                }
            } else {
                blend = tbl[motion].blend;
            }
        }
        cur = em->motionId[i];
        if (cur == id && em->motion[i].loop != 0 && em->motion[i].playing == 0) {
            blend = 0;
        }
        k = cur - 1000 - i * 200;
        if ((s = tbl[k].trans) != 0 && k > 0) {
            sub = &gEmMotionTransRows[em->type][s];
            id2 = sub->motion + 1000 + i * 200;
            if (id2 == cur) {
                frame = sub->frame;
                id = id2;
                if (em->rowBlend != 0) {
                    if (sub->blend != 0) {
                        blend = (s8) (s32) sub->blend;
                    } else {
                        blend = (s16) (s32) em->rowBlend;
                    }
                } else {
                    blend = sub->blend;
                }
                if (cur == id2 && em->motion[i].loop != 0) {
                    blend = 0;
                }
            }
        }
        if (cur == 0) {
            blend = 0;
        }
        em->motionId[i] = id;
        em->motionBlend[i] = blend / 2;
        em->motionFrame[i] = frame;
        Player_BeginLayerMotion((Player *)em, em->motionFrame[i], em->motionBlend[i], i);
    }
    em->rowBlend = tbl[motion].unk7;
}

/* unk_6FF630's .data (0x8B14A8-0x8BAC10) */
extern u32 gEmMotionRows_63[];
extern u32 gEmMotionRows_68[];
extern u32 D_008B9FA0[];
extern u32 D_008BA110[];
s16 AreaObj_K17_RectX[4] = {
#include "game/008B14A8.inc"
};
s16 AreaObj_K17_RectY[4] = {
#include "game/008B14B0.inc"
};
s16 AreaObj_K17_RectW[4] = {
#include "game/008B14B8.inc"
};
s16 AreaObj_K17_RectH[4] = {
#include "game/008B14C0.inc"
};
f32 AreaObj_K17_Spread1[3] = {
#include "game/008B14C8.inc"
};
s16 AreaObj_K17_AngleTbl[6] = {
#include "game/008B14D8.inc"
};
u8 AreaObj_K19_PointsA2[80] = {
#include "game/008B14F0.inc"
};
u8 AreaObj_K19_PointsA3[48] = {
#include "game/008B1540.inc"
};
u8 AreaObj_K19_PointsA6[32] = {
#include "game/008B1570.inc"
};
/* Area object kind 0xA7: five points (x, y, z). */
u8 AreaObj_K19_PointsA7[60] = {
#include "game/008B1590.inc"
};
/* Per monster type (gm_sub gm_em.c: Em +0x596). */
u8 D_008B15D0[74] = {
#include "game/008B15D0.inc"
};
/* Per monster type: its body scale (gm_sub gm_em.c: Em bodyScale and scaleX/Y/Z). */
f32 gEmBodyScales[74] = {
#include "game/008B1620.inc"
};
/* Per monster type: two floats, a minimum and a maximum (sub_main unk_3BE060.c). */
f32 gEffect01Sizes[74][2] = {
#include "game/008B1750.inc"
};
u32 gEffect06Scales[76] = {
#include "game/008B19A0.inc"
};
HitShape D_008B1AD0[1] = {
#include "game/008B1AD0.inc"
};
HitShape D_008B1B00[45] = {
#include "game/008B1B00.inc"
};
HitShape D_008B2210[3] = {
#include "game/008B2210.inc"
};
HitShape D_008B2290[2] = {
#include "game/008B2290.inc"
};
HitShape D_008B22E0[2] = {
#include "game/008B22E0.inc"
};
HitShape D_008B2330[26] = {
#include "game/008B2330.inc"
};
HitShape D_008B2740[41] = {
#include "game/008B2740.inc"
};
HitShape D_008B2DB0[20] = {
#include "game/008B2DB0.inc"
};
HitShape D_008B30D0[2] = {
#include "game/008B30D0.inc"
};
HitShape D_008B3120[32] = {
#include "game/008B3120.inc"
};
HitShape D_008B3620[3] = {
#include "game/008B3620.inc"
};
HitShape D_008B36A0[31] = {
#include "game/008B36A0.inc"
};
HitShape D_008B3B80[25] = {
#include "game/008B3B80.inc"
};
HitShape D_008B3F70[4] = {
#include "game/008B3F70.inc"
};
HitShape D_008B4010[32] = {
#include "game/008B4010.inc"
};
HitShape D_008B4510[2] = {
#include "game/008B4510.inc"
};
HitShape D_008B4560[24] = {
#include "game/008B4560.inc"
};
HitShape D_008B4920[32] = {
#include "game/008B4920.inc"
};
HitShape D_008B4E20[30] = {
#include "game/008B4E20.inc"
};
HitShape D_008B52D0[3] = {
#include "game/008B52D0.inc"
};
HitShape D_008B5350[32] = {
#include "game/008B5350.inc"
};
HitShape D_008B5850[4] = {
#include "game/008B5850.inc"
};
HitShape D_008B58F0[3] = {
#include "game/008B58F0.inc"
};
HitShape D_008B5970[4] = {
#include "game/008B5970.inc"
};
HitShape D_008B5A10[20] = {
#include "game/008B5A10.inc"
};
HitShape D_008B5D30[32] = {
#include "game/008B5D30.inc"
};
HitShape D_008B6230[21] = {
#include "game/008B6230.inc"
};
HitShape D_008B6580[13] = {
#include "game/008B6580.inc"
};
HitShape D_008B6790[21] = {
#include "game/008B6790.inc"
};
HitShape D_008B6AE0[36] = {
#include "game/008B6AE0.inc"
};
HitShape D_008B7080[43] = {
#include "game/008B7080.inc"
};
HitShape D_008B7740[2] = {
#include "game/008B7740.inc"
};
HitShape D_008B7790[2] = {
#include "game/008B7790.inc"
};
HitShape D_008B77E0[37] = {
#include "game/008B77E0.inc"
};
HitShape D_008B7DB0[40] = {
#include "game/008B7DB0.inc"
};
HitShape D_008B83F0[13] = {
#include "game/008B83F0.inc"
};
HitShape D_008B8600[14] = {
#include "game/008B8600.inc"
};
HitShape D_008B8830[25] = {
#include "game/008B8830.inc"
};
HitShape D_008B8C20[36] = {
#include "game/008B8C20.inc"
};
HitShape D_008B91C0[36] = {
#include "game/008B91C0.inc"
};
HitShape D_008B9760[2] = {
#include "game/008B9760.inc"
};
HitShape D_008B97B0[26] = {
#include "game/008B97B0.inc"
};
HitShape D_008B9BC0[3] = {
#include "game/008B9BC0.inc"
};
HitShape *gEmBoneHits[74] = {
    D_008B1AD0, D_008B3120, D_008B1B00, D_008B2210,
    D_008B2290, D_008B2290, D_008B2330, D_008B2740,
    D_008B2DB0, D_008B30D0, D_008B22E0, D_008B3120,
    D_008B3620, D_008B3F70, D_008B36A0, D_008B3B80,
    D_008B3F70, D_008B4010, D_008B1AD0, D_008B4510,
    D_008B4560, D_008B4920, D_008B4E20, D_008B30D0,
    D_008B22E0, D_008B52D0, D_008B5350, D_008B5850,
    D_008B5850, D_008B58F0, D_008B3F70, D_008B5850,
    D_008B2290, D_008B5970, D_008B5A10, D_008B3F70,
    D_008B1B00, D_008B3120, D_008B2330, D_008B4560,
    D_008B5D30, D_008B3120, D_008B3120, D_008B36A0,
    D_008B5350, D_008B3B80, D_008B4920, D_008B4010,
    D_008B6230, D_008B3120, D_008B2740, D_008B6580,
    D_008B6790, D_008B6580, D_008B6AE0, D_008B7080,
    D_008B7740, D_008B7790, D_008B77E0, D_008B7DB0,
    D_008B6AE0, D_008B83F0, D_008B8600, D_008B8830,
    D_008B8C20, D_008B91C0, D_008B9760, D_008B97B0,
    D_008B9BC0, D_008B2210, D_008B3620, D_008B1B00,
    D_008B77E0, D_008B9760,
};
u32 D_008B9D70[8] = {
#include "game/008B9D70.inc"
};
u32 D_008B9D90[8] = {
#include "game/008B9D90.inc"
};
u32 D_008B9DB0[8] = {
#include "game/008B9DB0.inc"
};
u32 D_008B9DD0[8] = {
#include "game/008B9DD0.inc"
};
u32 D_008B9DF0[8] = {
#include "game/008B9DF0.inc"
};
u32 D_008B9E10[8] = {
#include "game/008B9E10.inc"
};
u32 D_008B9E30[8] = {
#include "game/008B9E30.inc"
};
u32 D_008B9E50[8] = {
#include "game/008B9E50.inc"
};
u32 D_008B9E70[8] = {
#include "game/008B9E70.inc"
};
u32 D_008B9E90[8] = {
#include "game/008B9E90.inc"
};
u32 D_008B9EB0[4] = {
#include "game/008B9EB0.inc"
};
u32 D_008B9EC0[8] = {
#include "game/008B9EC0.inc"
};
u32 D_008B9EE0[8] = {
#include "game/008B9EE0.inc"
};
u32 D_008B9F00[8] = {
#include "game/008B9F00.inc"
};
u32 D_008B9F20[8] = {
#include "game/008B9F20.inc"
};
u32 D_008B9F40[8] = {
#include "game/008B9F40.inc"
};
u32 D_008B9F60[8] = {
#include "game/008B9F60.inc"
};
u32 D_008B9F80[8] = {
#include "game/008B9F80.inc"
};
u32 D_008B9FA0[84] = {
#include "game/008B9FA0.inc"
};
u32 D_008BA0F0[8] = {
#include "game/008BA0F0.inc"
};
u32 D_008BA110[84] = {
#include "game/008BA110.inc"
};
u8 gEmRows638_0[64] = {
#include "game/008BA260.inc"
};
u8 gEmRows638_2[64] = {
#include "game/008BA2A0.inc"
};
u8 gEmRows638_3[32] = {
#include "game/008BA2E0.inc"
};
u8 gEmRows638_4[32] = {
#include "game/008BA300.inc"
};
u8 gEmRows638_7[32] = {
#include "game/008BA320.inc"
};
u8 gEmRows638_8[64] = {
#include "game/008BA340.inc"
};
u8 gEmRows638_9[32] = {
#include "game/008BA380.inc"
};
u8 gEmRows638_12[32] = {
#include "game/008BA3A0.inc"
};
u8 gEmRows638_15[64] = {
#include "game/008BA3C0.inc"
};
u8 gEmRows638_13[64] = {
#include "game/008BA400.inc"
};
u8 gEmRows638_51[64] = {
#include "game/008BA440.inc"
};
u8 gEmRows638_54[64] = {
#include "game/008BA480.inc"
};
u8 gEmRows638_56[64] = {
#include "game/008BA4C0.inc"
};
u8 gEmRows638_58[64] = {
#include "game/008BA500.inc"
};
u8 gEmRows638_66[64] = {
#include "game/008BA540.inc"
};
u8 gEmRows638_73[64] = {
#include "game/008BA580.inc"
};
u8 *gEmRows638[76] = {
    gEmRows638_0, gEmRows638_0, gEmRows638_2, gEmRows638_3,
    gEmRows638_4, gEmRows638_4, gEmRows638_0, gEmRows638_7,
    gEmRows638_8, gEmRows638_9, gEmRows638_0, gEmRows638_0,
    gEmRows638_12, gEmRows638_13, gEmRows638_0, gEmRows638_15,
    gEmRows638_13, gEmRows638_0, gEmRows638_0, gEmRows638_0,
    gEmRows638_0, gEmRows638_0, gEmRows638_0, gEmRows638_9,
    gEmRows638_0, gEmRows638_12, gEmRows638_0, gEmRows638_13,
    gEmRows638_13, gEmRows638_0, gEmRows638_13, gEmRows638_13,
    gEmRows638_4, gEmRows638_3, gEmRows638_8, gEmRows638_13,
    gEmRows638_2, gEmRows638_0, gEmRows638_0, gEmRows638_0,
    gEmRows638_0, gEmRows638_0, gEmRows638_0, gEmRows638_0,
    gEmRows638_0, gEmRows638_15, gEmRows638_0, gEmRows638_0,
    gEmRows638_0, gEmRows638_0, gEmRows638_7, gEmRows638_51,
    gEmRows638_51, gEmRows638_51, gEmRows638_54, gEmRows638_0,
    gEmRows638_56, gEmRows638_9, gEmRows638_58, gEmRows638_0,
    gEmRows638_54, gEmRows638_51, gEmRows638_51, gEmRows638_0,
    gEmRows638_54, gEmRows638_54, gEmRows638_66, gEmRows638_0,
    gEmRows638_4, gEmRows638_3, gEmRows638_12, gEmRows638_2,
    gEmRows638_0, gEmRows638_73, NULL, NULL,
};
EmBendRow gEmBendOrigins_0 = {
#include "game/008BA6F0.inc"
};
EmBendRow gEmBendOrigins_7 = {
#include "game/008BA710.inc"
};
EmBendRow gEmBendOrigins_59 = {
#include "game/008BA730.inc"
};
EmBendRow *gEmBendOrigins[76] = {
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_7,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_7, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_59,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0, &gEmBendOrigins_0,
    &gEmBendOrigins_0, &gEmBendOrigins_0, NULL, NULL,
};
extern EmMotionRow D_0090F990[];
extern EmMotionRow D_00910010[];
extern EmMotionRow D_00910A00[];
extern EmMotionRow D_00911450[];
extern EmMotionRow D_00911910[];
extern EmMotionRow D_00912190[];
extern EmMotionRow D_00912650[];
extern EmMotionRow D_00912670[];
extern EmMotionRow D_00912C30[];
extern EmMotionRow D_009136D0[];
extern EmMotionRow D_00913BB0[];
extern EmMotionRow D_009142B0[];
extern EmMotionRow D_00914E90[];
extern EmMotionRow D_00915CA0[];
extern EmMotionRow D_00916340[];
extern EmMotionRow D_00917140[];
extern EmMotionRow D_009177E0[];
extern EmMotionRow D_00917CA0[];
extern EmMotionRow D_00918D50[];
extern EmMotionRow D_009197B0[];
extern EmMotionRow D_0091C480[];
extern EmMotionRow D_0091D120[];
extern EmMotionRow D_0091E1C0[];
extern EmMotionRow D_0091E700[];
extern EmMotionRow D_00920630[];
extern EmMotionRow D_00921E20[];
extern EmMotionRow D_009222E0[];
extern EmMotionRow D_009239A0[];
extern EmMotionRow D_009242E0[];
extern EmMotionRow D_009247A0[];
extern EmMotionRow D_00925150[];
extern EmMotionRow D_00926210[];
EmMotionRow *gEmMotionRows[76] = {
    D_0090F990, D_0090F990, D_00910010, D_00910A00,
    D_00911450, D_00911450, D_00911910, D_00912190,
    D_00912C30, D_009136D0, D_00913BB0, D_0090F990,
    D_009142B0, D_00916340, D_00914E90, D_00915CA0,
    D_00916340, D_00917140, D_009177E0, D_00917CA0,
    D_00918D50, D_009197B0, D_00917140, D_009136D0,
    D_00917CA0, D_009142B0, D_00914E90, D_00916340,
    D_00916340, D_0091C480, D_00916340, D_00916340,
    D_00911450, D_00910A00, D_00912C30, D_00916340,
    D_0091D120, D_0090F990, D_00911910, D_00918D50,
    D_0091E1C0, D_0090F990, D_0090F990, D_00914E90,
    D_00914E90, D_0091E700, D_009197B0, D_00917140,
    D_00920630, D_0090F990, D_00912190, D_00921E20,
    D_009222E0, D_00921E20, D_009239A0, D_009242E0,
    D_0090F990, D_009247A0, D_00925150, D_00926210,
    D_009239A0, D_00921E20, D_009222E0, (EmMotionRow *)gEmMotionRows_63,
    D_009239A0, D_009239A0, D_00920630, D_00920630,
    (EmMotionRow *)gEmMotionRows_68, D_00910A00, D_009142B0, D_0091D120,
    D_0090F990, D_00920630, NULL, NULL,
};
EmMotionRow *gEmMotionTransRows[76] = {
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, D_00912650, D_00912650,
    D_00912650, D_00912650, NULL, NULL,
};
EmMotionRow *gEmMotionAltRows[76] = {
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, D_00912670, D_00912670,
    D_00912670, D_00912670, NULL, NULL,
};
