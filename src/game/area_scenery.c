/*
 * The area objects of the later kinds: objects of the area's own list (Obj,
 * from Obj_Alloc) that
 *
 * Area_StartObjects starts when an area is entered, by the area's id or by
 * gAreaInfo's set-up ids, and
 *
 * the types 7 to 11 of AmbientLife, which AmbientLife_Setup, AmbientLife_Run
 * and AmbientLife_Draw call
 *
 * by Obj.type. Each kind has a Create function, an Update that runs the state
 * Obj.state names (0 set
 *
 * up, 1 run, 2 end, 3 free) and a Draw that its draw slot calls
 * (DrawSlot_Alloc; the run state adds
 *
 * the slot to a depth-sorted list with OT_AddByDepth every frame). The models
 * come from the area's
 *
 * sub-model (gAreaSubModels[0]) or the area model (gAreaState.model). The
 * kinds: FortressTremor,
 *
 * BreakableStructure, ScrollingOverlay, TurningProp, GroundTileGrid,
 * AnimatedScenery, CastleGate and
 *
 * DistantLightning. The file ends with their tables of positions, models and
 * texture steps. The Em
 *
 * views and prototypes declared at its top (EmReaction, EmReactView,
 * EmSenseView, EmSight,
 *
 * EmBendView) are used by no function here.
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
    s32 noticeAdd;
    s32 noticeMax;
} EmReaction;

extern EmRate *gEmRateTbl[];

/* A view of Em at its per-player reaction values: four s16, one per player, then their last-seen
 * copies (Em_UpdateReactions runs the reaction of a player whose value changed). */
typedef struct EmReactView {
    u8 pad[0x6C4];
    s16 cur[4];
    s16 last[4];
} EmReactView;

/* A view of Em at what Em_SensePlayers measures of each player every frame. */
typedef struct EmSenseView {
    u8 pad[0x7E4];
    s16 noticeTimer[4];              /* 0x7E4: Em unk7E4 */
    u8 pad7EC[0x818 - 0x7EC];
    f32 dist[4];                /* 0x818: from the monster to the player */
    f32 distXZ[4];              /* 0x828: the same, in the ground plane */
    f32 range[4];               /* 0x838: the four reaction ranges (Em_GetReaction kinds 3 to 6) */
    s32 alertBuildup[4];              /* 0x848: Em unk848, cleared when the player is out of reach */
    u16 dir[4];                 /* 0x858: the direction to the player (Math_AngleXZ) */
} EmSenseView;

/* prototypes */
void ScrollingOverlay_Draw(DrawSlot *ds);
extern f32 CastleGate_SoundPos[];
extern f32 D_008B149C[3];
void StageShot_CreateAt(f32 *pos, u8 kind, u8 area, s32 ang);
void CamShake_SetA(s32 kind, f32 *pos);
extern s16 AreaObj_K17_AngleTbl[];
void AttackBurst_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
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
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
extern f32 BreakableStructure_SoundPos[];
extern f32 D_008B0B8C[];
extern f32 BreakableStructure_EffectPos[][3];
extern EmObj *gFlashObjs[32];
u16 Math_AngleXZ(f32 *from, f32 *to);
typedef struct EmSight {
    f32 *pos;
    s32 target;
    s32 dir;
    u16 half;
} EmSight;
f32 Math_Cos(f32 x);
f32 Math_Absf(f32 x);
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask);
s32 Cyl_Contains(f32 *p, f32 *c, f32 r, f32 top, f32 bottom);
f32 Math_Sin(f32 x);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
extern s16 BreakableStructure_ModelsDay[];
extern s16 BreakableStructure_ModelsNight[];
extern u16 BreakableStructure_Yaw[];
extern f32 AmbientLife_CrossingFlyerPoints[];
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
extern f32 FortressTremor_ImpactOffsets[];
extern f32 D_008B0AC4[];
extern f32 D_008B0AC8[];
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void CamShake_SetAEverywhere(s32);
extern f32 CastleGate_PosOpen[][3];
extern f32 CastleGate_PosShut[][3];
u8 Player_InCurrentArea(void *obj);
void CastleGate_Draw(DrawSlot *ds);
typedef struct BreakableStructurePiece {
    s16 slot;
    s16 idx;
    f32 pos[3];
    u16 ang;
    u8 pad12[2];
    DrawSlot *ds;
} BreakableStructurePiece;
extern f32 BreakableStructure_Pos[][3];
extern f32 BreakableStructure_Rise[];
extern s16 BreakableStructure_Tilt[];
void BreakableStructure_Draw(DrawSlot *s);
u8 Quest_GetSeedByte(u8 i);
void PlayerFx_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void VolcanoHazard_Create(f32 *pos, u8 k, u8 c);
s32 AreaObj_K19_GetPoints(u16 arg0);
s16 AreaObj_K19_PointCount(u16 arg0);
extern f32 AmbientLife_GlowBugTowerPos[];
extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateY(f32 *v, f32 a);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 *AnimatedScenery_Positions[];
void AnimatedScenery_Draw(DrawSlot *ds);
void CastleGate_Update(Obj *o);
extern f32 FortressTremor_PosX;
extern f32 D_008B0AAC;
extern f32 FortressTremor_PosZ;
void AreaObj_K17_Draw(DrawSlot *slot);
void FortressTremor_Draw(DrawSlot *ds);
void DistantLightning_Draw(DrawSlot *ds);
extern f32 TurningProp_Pos[][3];
void Vec_Copy(f32 *dst, f32 *src);
void TurningProp_Update(Obj *o);
void TurningProp_Draw(DrawSlot *ds);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void GroundTileGrid_Draw(DrawSlot *s);
f32 *Mtx_FromAnglesXYZ(s32 *ang, f32 *mtx);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
void Mtx_TurnZ(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
extern s16 TurningProp_Models[];
extern f32 TurningProp_Scales[];
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
void AmbientLife_DrawModel(ModelMatSet *set, s32 arg1);
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
void AmbientLife_ResetGlowBug(Obj *o);
void FortressTremor_Setup(Obj *o);
void FortressTremor_Run(Obj *o);
void FortressTremor_End(Obj *o);
void FortressTremor_Free(Obj *o);
void BreakableStructure_Setup(Obj *o);
void BreakableStructure_Run(Obj *o);
void BreakableStructure_End(Obj *o);
void BreakableStructure_Free(Obj *o);
void ScrollingOverlay_Setup(Obj *o);
void ScrollingOverlay_Run(Obj *o);
void ScrollingOverlay_End(Obj *o);
void ScrollingOverlay_Free(Obj *o);
void TurningProp_Setup(Obj *o);
void TurningProp_Run(Obj *o);
void TurningProp_End(Obj *o);
void TurningProp_Free(Obj *o);
void GroundTileGrid_Setup(Obj *o);
u32 GroundTileGrid_Run(Obj *o);
void GroundTileGrid_End(Obj *o);
void GroundTileGrid_Free(Obj *o);
void AnimatedScenery_Setup(Obj *o);
u32 AnimatedScenery_Run(Obj *o);
void AnimatedScenery_End(Obj *o);
void AnimatedScenery_Free(Obj *o);
void CastleGate_Setup(Obj *o);
void CastleGate_Run(Obj *o);
void CastleGate_End(Obj *o);
void CastleGate_Free(Obj *o);
void DistantLightning_Setup(Obj *o);
void DistantLightning_Run(Obj *o);
void DistantLightning_End(Obj *o);
void DistantLightning_Free(Obj *o);
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
void FortressTremor_Update(Obj *o);
void BreakableStructure_Update(Obj *o);
void ScrollingOverlay_Update(Obj *o);
void GroundTileGrid_Update(Obj *o);
void DistantLightning_Update(Obj *o);
void AreaObj_K19_Update(Obj *o);
void AnimatedScenery_Update(Obj *o);
void AreaObj_K17_Update(Obj *o);
s32 Model_Draw(u32 h, s32 flags);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 reg, u32 v);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern EmObj *gAreaObjList[32];
u32 Player_TestFlag(void *obj, u32 flag);
f32 Em_LevelAttackFactor(Em *em);
f32 Em_LevelDamageFactor(Em *em);
extern EmRageRow *gEmRageTbl[];
extern s16 CastleGate_Models[];
void Em_SyncState(Em *em, s32 kind, s32 b, s32 c);
s32 Game_IsOnline(void);
extern s32 *gEmPartBuildupLimits[];
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
u16 Area_DayId(u16 id);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
extern u8 GroundTileGrid_Jungle1Tiles[];
extern u8 GroundTileGrid_Jungle6Tiles[];
extern u16 GroundTileGrid_Jungle6Turns[];
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
/*
 * Run state of AmbientLife's circling flyer (type 7): adds its turn step
 * (AmbientLifeMover.circleStep) to its heading (AmbientLifeMover.rot[1]), places the draw slot
 * at the circle's centre (Obj.pos) plus the circle offset kept in AmbientLifeMover.m
 * turned by that heading, and adds the slot to gDrawOt32. Returns
 * OT_AddByDepth's result.
 */
u32 AmbientLife_RunCirclingFlyer(Obj *obj)
{
    u8 *tumble = obj->buf;
    f32 offset[4];

    *(u16 *) (tumble + 0x4E) += (s32) *(u16 *) (tumble + 0x54);
    Mtx_GetTrans(offset, (f32 *)tumble);
    Vec_RotateY(offset, (6.2831855f * *(u16 *) (tumble + 0x4E)) / 65536.0f);
    obj->ds->pos[0] = obj->pos[0] + offset[0];
    obj->ds->pos[1] = obj->pos[1] + offset[1];
    obj->ds->pos[2] = obj->pos[2] + offset[2];
    return OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/*
 * Draws the circling flyer: model 8 of the area's ambient set
 * (gAreaFxModels) from the area's sub-model, turned by its heading and its
 * facing (AmbientLifeMover.m), at the draw slot, banked 10 degrees about z into the
 * turn (Obj.var2 is the direction); AREA_FOREST_HILLS_6 draws it three times
 * bigger. Draws nothing without the sub-model or the model. The switch keeps
 * default first: the compiler then lays out the branches, with their empty
 * delay slots, as the game's code does.
 */
void AmbientLife_DrawCirclingFlyer(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 *orient = obj->buf;
    ModelMatSet *model;
    f32 mtx[16];
    s16 k;
    u16 area;

    if (stage != NULL) {
        if (stage->active == 0) {
            return;
        }
        area = gScene.area;
        k = gAreaFxModels[area][8];
        if (k >= 0) {
            model = &stage->matSets[k];
            switch (area) {
            default:
                Mtx_Identity(mtx);
                break;
            case AREA_FOREST_HILLS_6:
                Mtx_Scale(mtx, 3.0f, 3.0f, 3.0f);
                break;
            }
            Mtx_RotateY(mtx, (6.2831855f * *(u16 *) ((u8 *) orient + 0x4E)) / 65536.0f);
            Mtx_Concat3x3(mtx, orient);
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            if (obj->var2 == 0) {
                Mtx_TurnZ(mtx, -0.17453294f);
            } else {
                Mtx_TurnZ(mtx, 0.17453294f);
            }
            AmbientLife_DrawModel(model, (s32) mtx);
        }
    }
}

/*
 * Starts a life of AmbientLife's glow bug (type 8): unless it was placed at a
 * gathering point (Obj.var4 0xFF), moves it near a fixed point of
 * AREA_FOREST_HILLS_9 and its night, or to its place Obj.var4 of
 * AmbientLife_GlowBugTowerPos in AREA_TOWER_8; then picks a life of 5 to 8
 * seconds (Obj.var3), its drift directions on x and z (Obj.var1, Obj.var2)
 * and full brightness (Obj.fvar 1.0).
 */
void AmbientLife_ResetGlowBug(Obj *obj)
{

    if (obj->var4 != 0xFF) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_9:
        case AREA_FOREST_HILLS_NIGHT_9:
            obj->pos[0] = 11750.0f + (f32) ((Rand_Next(1) & 0x1FF) - 0x100);
            obj->pos[1] = 200.0f;
            obj->pos[2] = 9500.0f + (f32) ((Rand_Next(1) & 0x1FF) - 0x100);
            break;
        case AREA_TOWER_8:
            obj->pos[0] = AmbientLife_GlowBugTowerPos[obj->var4 * 3];
            obj->pos[1] = D_008B0A14[obj->var4 * 3];
            obj->pos[2] = D_008B0A18[obj->var4 * 3];
            break;
        }
    }
    obj->timer = 0;
    obj->var3 = ((Rand_Next(1) & 3) + 5) * 60;
    obj->var1 = Rand_Next(1) & 1;
    obj->var2 = Rand_Next(1) & 1;
    obj->fvar = 1.0f;
}

/* Set-up of the glow bug: AmbientLife_ResetGlowBug. */
void AmbientLife_SetupGlowBug(Obj *obj)
{
    AmbientLife_ResetGlowBug(obj);
}

/*
 * Run state of the glow bug: while it lives (Obj.var0 0) its brightness
 * Obj.fvar pulses (|cos| of the frame count, a turn per 64 frames) and it
 * drifts 0.2 to 1.5 a frame on x and z, rising by their mean; then it dims by
 * a tenth a frame for 10 frames and starts a new life, or ends when it came
 * from a gathering point. Adds the draw slot to gDrawOtSorted.
 */
void AmbientLife_RunGlowBug(Obj *obj)
{
    f32 x;
    f32 z;

    switch (obj->var0) {
    case 0:
        obj->timer++;
        if (obj->timer >= obj->var3) {
            obj->var0++;
            obj->timer = 10;
        } else {
            obj->fvar = Math_Absf(Math_Cos((6.2831855f * (obj->timer << 10)) / 65536.0f));
        }
        x = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        z = 0.2f + 0.02f * (Rand_Next(1) & 0x3F);
        if (obj->var1) {
            x *= -1.0f;
        }
        if (obj->var2) {
            z *= -1.0f;
        }
        obj->pos[0] += x;
        obj->pos[2] += z;
        obj->pos[1] += (x + z) / 2.0f;
        break;
    case 1:
        if (--obj->timer <= 0) {
            if (obj->var4 == 0xFF) {
                obj->state++;
                return;
            }
            obj->var0 = 0;
            AmbientLife_ResetGlowBug(obj);
        } else {
            obj->fvar -= obj->fvar / 10.0f;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
}

/*
 * Draws the glow bug facing the camera: model 9 of the area's ambient set,
 * blended by Gfx_SetBlend(4, 1) without depth writes, scaled by its
 * brightness (not in AREA_TOWER_8), with alpha 255 * Obj.fvar; then restores
 * the blend and depth writes.
 */
void AmbientLife_DrawGlowBug(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 mtx[16];
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = gAreaFxModels[gScene.area][9];
        if (k >= 0) {
            model = &stage->matSets[k];
            Gfx_SetBlend(4, 1);
            Gs_SetState(0x60, 0);
            Gs_SetState(0x6C, 0);
            if (gScene.area == AREA_TOWER_8) {
                Mtx_Identity(mtx);
            } else {
                Mtx_Scale(mtx, obj->fvar, obj->fvar, obj->fvar);
            }
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Gs_SetState(0x67, ((u8) (u32) (255.0f * obj->fvar) << 24) | 0xFFFFFF);
            AmbientLife_DrawModel(model, (s32) mtx);
            Gfx_SetBlend(4, 5);
            Gs_SetState(0x6C, 1);
        }
    }
}

/* Set-up of AmbientLife's type 9, which no spawner creates: does nothing. */
void AmbientLife_SetupUnused(void)
{
}

/* Run state of the unused type 9: does nothing. */
void AmbientLife_RunUnused(void)
{
}

/* Draw of the unused type 9: does nothing. */
void AmbientLife_DrawUnused(void)
{
}

/* Set-up of AmbientLife's type 10, which no spawner creates: does nothing. */
void AmbientLife_SetupUnusedNext(void)
{
}

/* Run state of the unused type 10: does nothing. */
void AmbientLife_RunUnusedNext(void)
{
}

/* Draw of the unused type 10: does nothing. */
void AmbientLife_DrawUnusedNext(void)
{
}

/*
 * Set-up of AmbientLife's crossing flyer (type 11), which arrives with the
 * number of its points in Obj.var3 (2, from AmbientLife_SpawnChildWithPoints): picks a
 * start point (Obj.var0) and another end point (Obj.var1) of
 * AmbientLife_CrossingFlyerPoints, a bob height of 12.5 to 37.5 (Obj.fvar)
 * and a random phase (Obj.timer). In AREA_FOREST_HILLS_3 and its night it
 * starts at its point (height jittered by up to 128) heading for the end
 * point (Obj.var3). Obj.var0 then becomes half its bob period, 48 to 79
 * frames.
 */
void AmbientLife_SetupCrossingFlyer(Obj *obj)
{

    obj->var0 = Rand_Next(1) % obj->var3;
    obj->var1 = Rand_Next(1) % (obj->var3 - 1) + 1;
    obj->var1 = obj->var1 + obj->var0;
    if (obj->var1 >= obj->var3) {
        obj->var1 -= obj->var3;
    }
    obj->fvar = 25.0f * (0.5f + 0.001f * (Rand_Next(1) & 0x3FF));
    obj->var2 = 0;
    obj->timer = Rand_Next(1);
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        obj->pos[0] = AmbientLife_CrossingFlyerPoints[obj->var0 * 3];
        obj->pos[1] = D_008B0A94[obj->var0 * 3] + (f32) ((u8) Rand_Next(1) - 0x80);
        obj->pos[2] = D_008B0A98[obj->var0 * 3];
        obj->var3 = Math_Heading(AmbientLife_CrossingFlyerPoints[obj->var1 * 3], D_008B0A98[obj->var1 * 3], obj->pos[0], obj->pos[2]) + 0x4000;
        break;
    }
    obj->var0 = (Rand_Next(1) & 0x1F) + 0x30;
}

/*
 * Run state of the crossing flyer: steps its wing frame (Obj.var2, 0-3); ends
 * once within 100 of its end point; else moves 10 a frame along its heading
 * and places the draw slot bobbing by Obj.fvar times a sine of the frame
 * count (a wave per 2 * Obj.var0 frames), added to gDrawOt32.
 */
void AmbientLife_RunCrossingFlyer(Obj *obj)
{
    f32 step[4];
    f32 target[4];
    f32 *stepY;
    f32 *stepZ;
    f32 wave;
    f32 bob;

    obj->timer++;
    obj->var2++;
    obj->var2 &= 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        target[0] = AmbientLife_CrossingFlyerPoints[obj->var1 * 3];
        target[1] = D_008B0A94[obj->var1 * 3];
        target[2] = D_008B0A98[obj->var1 * 3];
        break;
    }
    if (Cyl_Contains(obj->pos, target, 100.0f, -1.0f, -1.0f) != 0) {
        obj->state++;
        return;
    }
    step[0] = 0.0f;
    stepY = &step[1];
    stepZ = &step[2];
    *stepY = 0.0f;
    *stepZ = 10.0f;
    Vec_RotateY(step, (6.2831855f * obj->var3) / 65536.0f);
    obj->pos[0] += step[0];
    obj->pos[1] += *stepY;
    obj->pos[2] += *stepZ;
    wave = Math_Sin((6.2831855f * (360.0f * ((f32) obj->timer / (f32) (obj->var0 * 2)))) / 360.0f);
    bob = obj->fvar * wave;
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1] + bob;
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/*
 * Draws the crossing flyer: model 8 + Obj.var2 of the area's sub-model (its
 * four wing frames) at the draw slot, turned by its heading, with alpha test
 * reference 0x80.
 */
void AmbientLife_DrawCrossingFlyer(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = obj->var2;
        mdl = stage->matSets + k + 8;
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
        AmbientLife_DrawModel(mdl, (s32) mtx);
    }
}

/*
 * Creates the FortressTremor object (kind 0xA, no buffer); Area_StartObjects
 * calls it for AREA_FORTRESS_0 and its night.
 */
void FortressTremor_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0xA;
        obj->type = 0;
        obj->update = FortressTremor_Update;
    }
}

/*
 * FortressTremor's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void FortressTremor_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        FortressTremor_Setup(obj);
        break;
    case 1:
        FortressTremor_Run(obj);
        break;
    case 2:
        FortressTremor_End(obj);
        break;
    case 3:
        FortressTremor_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the object at FortressTremor_PosX,
 * FortressTremor_StartY and FortressTremor_PosZ, takes a draw slot
 * (FortressTremor_Draw) and runs the first frame, which starts a tremor at
 * once; frees the object when no slot is free.
 */
void FortressTremor_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->pos[0] = FortressTremor_PosX;
    obj->pos[1] = D_008B0AAC;
    obj->pos[2] = FortressTremor_PosZ;
    obj->timer = 0;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = FortressTremor_Draw;
        FortressTremor_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: when its wait (Obj.timer) runs out, a tremor: shakes the camera
 * for everyone, plays the rumble (area sound 0x60) at the local player and
 * drops its model 62.5 a frame for 15 frames (drawn through gDrawOt32),
 * playing an impact (area sound 0x61) at one of four
 * FortressTremor_ImpactOffsets around the player on the third; then waits 60
 * to 91 frames back at its start height.
 */
void FortressTremor_Run(Obj *obj)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 impactPos[4];
    s16 k;

    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            obj->var0++;
            obj->timer = 16;
            CamShake_SetAEverywhere(0);
            Sound_PlaySeAtPosChecked(7, 0x60, 0, pl->pos, 0x13, 0);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
            obj->timer = (Rand_Next(1) & 0x1F) + 0x3C;
            obj->pos[1] = D_008B0AAC;
            break;
        }
        if (obj->timer == 0xD) {
            k = Rand_Next(1) & 3;
            impactPos[0] = pl->pos[0] + FortressTremor_ImpactOffsets[k * 3];
            impactPos[1] = pl->pos[1] + D_008B0AC4[k * 3];
            impactPos[2] = pl->pos[2] + D_008B0AC8[k * 3];
            Sound_PlaySeAtPosChecked(7, 0x61, 0, impactPos, 0x13, 0);
        }
        obj->pos[1] -= 62.5f;
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 1);
        break;
    }
}

/* End state: hides the object; its draw slot is not freed. */
void FortressTremor_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void FortressTremor_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the first model of the area's sub-model at the draw slot, alpha test
 * reference 0x80.
 */
void FortressTremor_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    if ((subs != NULL) && (subs->active != 0)) {
        model = subs->matSets;
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        if (model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the BreakableStructure object (kind 0xB, one buffer block for its
 * five BreakableStructurePiece entries); Area_StartObjects calls it for
 * AREA_FORTRESS_3 and its night.
 */
void BreakableStructure_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0xB;
        obj->type = 0;
        obj->update = BreakableStructure_Update;
    }
}

/*
 * BreakableStructure's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void BreakableStructure_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        BreakableStructure_Setup(obj);
        break;
    case 1:
        BreakableStructure_Run(obj);
        break;
    case 2:
        BreakableStructure_End(obj);
        break;
    case 3:
        BreakableStructure_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the five pieces (BreakableStructure_Pos); when bit 0
 * of gScene.emEventFlags is already set it starts broken (Obj.var0 2), each
 * piece raised by BreakableStructure_Rise and tilted by
 * BreakableStructure_Tilt. Gives each piece a draw slot
 * (BreakableStructure_Draw, DrawSlot.part its index; none when the pool is
 * full) and runs the first frame.
 */
void BreakableStructure_Setup(Obj *obj)
{
    BreakableStructurePiece *piece = obj->buf;
    s16 i;

    obj->state++;
    if (gScene.emEventFlags & 1) {
        obj->var0 = 2;
    } else {
        obj->var0 = 0;
    }
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->timer = 0;
    for (i = 0; i < 5; i++, piece++) {
        piece->idx = i;
        piece->ang = 0;
        Vec_Copy(piece->pos, BreakableStructure_Pos[i]);
        if (obj->var0 == 2) {
            piece->pos[1] += BreakableStructure_Rise[i];
            piece->ang += (s32) (65536.0f * BreakableStructure_Tilt[i] / 360.0f + 0.5f);
        }
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->ds = DrawSlot_Get(piece->slot);
            piece->ds->owner = obj;
            piece->ds->part = i;
            piece->ds->draw = BreakableStructure_Draw;
        } else {
            piece->ds = NULL;
        }
    }
    BreakableStructure_Run(obj);
}

/*
 * Run state: whole (Obj.var0 0), it waits for bit 0 of gScene.emEventFlags (a
 * fortress strike of Lao-Shan Lung or Shen Gaoren), then plays the break
 * sounds (0x1A, 0x1B) of the first such monster shown in the area and spawns
 * three effects (Effect0D kind 0, scale 25); for the next 5 frames each piece
 * rises and tilts by a fifth of its amounts; then it stays broken. Draws
 * pieces 0-3 while whole, all five while breaking, piece 4 alone once broken
 * (by the step at the frame's start).
 */
void BreakableStructure_Run(Obj *obj)
{
    BreakableStructurePiece *piece = obj->buf;
    Em *em = gEms;
    s16 stepAtStart = obj->var0;
    s16 i;

    switch (obj->var0) {
    case 0:
        if (gScene.emEventFlags & 1) {
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0
                    && (em->type == 7 || em->type == 0x32 || em->type == 0x37)) {
                    Em_PlaySeChecked(em, 0x1A, 0, BreakableStructure_SoundPos, 8, 0);
                    Em_PlaySeChecked(em, 0x1B, 0, D_008B0B8C, 8, 0);
                    break;
                }
            }
            for (i = 0; i < 3; i++) {
                PartFx_SpawnAt(BreakableStructure_EffectPos[i], 0, 25.0f);
            }
            obj->var0++;
        }
        break;
    case 1:
        if (++obj->timer >= 5) {
            obj->var0++;
        }
        break;
    case 2:
        break;
    }
    for (i = 0; i < 5; i++, piece++) {
        switch (stepAtStart) {
        case 0:
            if (piece->idx >= 4) {
                continue;
            }
            break;
        case 1:
            piece->pos[1] += BreakableStructure_Rise[piece->idx] / 5.0f;
            piece->ang += (s32) (65536.0f * (BreakableStructure_Tilt[piece->idx] / 5) / 360.0f + 0.5f);
            break;
        case 2:
            if (piece->idx < 4) {
                continue;
            }
            break;
        }
        if (piece->ds != NULL) {
            piece->ds->pos[0] = piece->pos[0];
            piece->ds->pos[1] = piece->pos[1];
            piece->ds->pos[2] = piece->pos[2];
            OT_AddByDepth(gDrawOt32, piece->ds, 0x20, 0);
        }
    }
}

/*
 * End state: hides the object and frees the draw slots its pieces hold (the
 * piece entries read by byte offset).
 */
void BreakableStructure_End(Obj *obj)
{
    u8 *pieceBytes = obj->buf;
    s16 i;

    obj->state++;
    obj->visible = 0;
    for (i = 0; i < 5; i++) {
        if (*(s32 *) (pieceBytes + 0x14) != 0) {
            DrawSlot_Free(*(s16 *) pieceBytes);
        }
        pieceBytes += 0x18;
    }
}

/* Free state: Obj_Free. */
void BreakableStructure_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws piece DrawSlot.part: its model (BreakableStructure_ModelsDay in
 * AREA_FORTRESS_3, BreakableStructure_ModelsNight in AREA_FORTRESS_NIGHT_3,
 * nothing elsewhere) at the slot, tilted about x by the piece's angle and
 * turned by BreakableStructure_Yaw, alpha test reference 0x80; skips a model
 * whose id is 0.
 */
void BreakableStructure_Draw(DrawSlot *drawSlot)
{
    BreakableStructurePiece *piece = &((BreakableStructurePiece *) ((Obj *) drawSlot->owner)->buf)[drawSlot->part];
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;

    if (stage != NULL && stage->active != 0) {
        switch (gScene.area) {
        case AREA_FORTRESS_3:
            mdl = &stage->matSets[BreakableStructure_ModelsDay[piece->idx]];
            break;
        case AREA_FORTRESS_NIGHT_3:
            mdl = &stage->matSets[BreakableStructure_ModelsNight[piece->idx]];
            break;
        default:
            return;
        }
        Gs_SetState(0x60, 0x80);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * piece->ang) / 65536.0f, (6.2831855f * BreakableStructure_Yaw[piece->idx]) / 65536.0f, 0.0f);
        Gs_SetState(0x1A, (s32) mtx);
        if (mdl != NULL && mdl->id != 0) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x60, 0);
            Draw_SetDefaultState();
        }
    }
}

/*
 * Creates the ScrollingOverlay object (kind 0xE, no buffer);
 * Area_StartObjects calls it in the areas its set-up knows.
 */
void ScrollingOverlay_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0xE;
        obj->type = 0;
        obj->update = ScrollingOverlay_Update;
    }
}

/*
 * ScrollingOverlay's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void ScrollingOverlay_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        ScrollingOverlay_Setup(obj);
        break;
    case 1:
        ScrollingOverlay_Run(obj);
        break;
    case 2:
        ScrollingOverlay_End(obj);
        break;
    case 3:
        ScrollingOverlay_Free(obj);
        break;
    }
}

/*
 * Set-up state: picks the type by Area_DayId of the area (0
 * AREA_FOREST_HILLS_11, 1 AREA_DESERT_NIGHT_7, 2 AREA_GREAT_ARENA_1, 3
 * AREA_DESERT_2, 4 AREA_DESERT_3, 5 AREA_DESERT_9, 6 AREA_DESERT_10, with
 * most nights) and its position. Types 1-5 pick the rows their two sliding
 * overlays start on (Obj.var0, Obj.var1, never the same), the overlays'
 * durations (Obj.var2, Obj.var4: 55 to 64 frames) and starting frames
 * (Obj.timer, Obj.var3); type 6 a 100-frame cycle (Obj.var3). Frees the
 * object in other areas; else takes a draw slot (ScrollingOverlay_Draw) and
 * runs the first frame, or frees it when none is free.
 */
void ScrollingOverlay_Setup(Obj *obj)
{
    DrawSlot *drawSlot;

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->timer = 0;
    switch (Area_DayId(gScene.area)) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        obj->type = 0;
        obj->pos[0] = 7227.0f;
        obj->pos[1] = -34.0f;
        obj->pos[2] = 5795.0f;
        break;
    case AREA_DESERT_NIGHT_7:
        obj->type = 1;
        obj->pos[0] = 8800.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10400.0f;
        obj->var0 = Rand_Next(1) % 5;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 4 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 5) {
            obj->var1 -= 5;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        obj->type = 2;
        obj->pos[0] = 9400.0f;
        obj->pos[1] = 18.0f;
        obj->pos[2] = 9780.0f;
        obj->var0 = Rand_Next(1) % 5;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 4 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 5) {
            obj->var1 -= 5;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_2:
        obj->type = 3;
        obj->pos[0] = 6200.0f;
        obj->pos[1] = 54.0f;
        obj->pos[2] = 5500.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_3:
        obj->type = 4;
        obj->pos[0] = 10000.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10300.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_9:
    case AREA_DESERT_9:
        obj->type = 5;
        obj->pos[0] = 10000.0f;
        obj->pos[1] = 13.0f;
        obj->pos[2] = 10300.0f;
        obj->var0 = Rand_Next(1) % 4;
        obj->timer = Rand_Next(1) & 0xF;
        obj->var2 = Rand_Next(1) % 10 + 55;
        obj->var1 = Rand_Next(1) % 3 + 1;
        obj->var1 += obj->var0;
        if (obj->var1 >= 4) {
            obj->var1 -= 4;
        }
        obj->var3 = Rand_Next(1) & 0xF;
        obj->var4 = Rand_Next(1) % 10 + 55;
        break;
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        obj->type = 6;
        obj->var3 = 100;
        break;
    default:
        Obj_Free(obj);
        return;
    }
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = ScrollingOverlay_Draw;
        ScrollingOverlay_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: in types 1-5, when an overlay's duration runs out, restarts it
 * on a random row other than the other overlay's, with a new duration
 * (overlay 0: Obj.timer, Obj.var2, row Obj.var0; overlay 1: Obj.var3,
 * Obj.var4, row Obj.var1); type 6 counts Obj.timer through 100 frames, the
 * others count it up. Adds the draw slot to gDrawOt8 while Obj.draw is NULL
 * (always: the set-up clears it).
 */
void ScrollingOverlay_Run(Obj *obj)
{

    switch (obj->type) {
    case 1:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 4 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 5) {
                obj->var0 -= 5;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 4 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 5) {
                obj->var1 -= 5;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 2:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 4 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 5) {
                obj->var0 -= 5;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 4 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 5) {
                obj->var1 -= 5;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 3:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 4:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 5:
        if (++obj->timer > obj->var2) {
            obj->var0 = Rand_Next(1) % 3 + 1;
            obj->var0 += obj->var1;
            if (obj->var0 >= 4) {
                obj->var0 -= 4;
            }
            obj->timer = 0;
            obj->var2 = Rand_Next(1) % 10 + 55;
        }
        if (++obj->var3 > obj->var4) {
            obj->var1 = Rand_Next(1) % 3 + 1;
            obj->var1 += obj->var0;
            if (obj->var1 >= 4) {
                obj->var1 -= 4;
            }
            obj->var3 = 0;
            obj->var4 = Rand_Next(1) % 10 + 55;
        }
        break;
    case 6:
        if (++obj->timer >= obj->var3) {
            obj->timer = 0;
            obj->var3 = 100;
        } else if (obj->timer >= 100) {
            return;
        }
        break;
    default:
        obj->timer++;
        break;
    }
    if (obj->draw == NULL) {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
    }
}

/* End state: hides the object; its draw slot is not freed. */
void ScrollingOverlay_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void ScrollingOverlay_Free(Obj *obj)
{
    Obj_Free(obj);
}

extern f32 ScrollingOverlay_FlipbookUV[16][2];
extern f32 ScrollingOverlay_ForestHillsPos[4][3];
extern f32 ScrollingOverlay_RowsDesertNight7[5][3];
extern f32 ScrollingOverlay_RowsGreatArena[5][3];
extern f32 ScrollingOverlay_RowsDesert2[4][3];
extern f32 ScrollingOverlay_RowsDesert3[4][3];
extern f32 ScrollingOverlay_RowsDesert9[4][3];
extern s16 ScrollingOverlay_SlotsForestHills[4];
extern s16 ScrollingOverlay_SlotsDesertNight7[4];
extern s16 ScrollingOverlay_SlotsGreatArena[4];
extern s16 ScrollingOverlay_SlotsDesert2[4];
extern s16 ScrollingOverlay_SlotsDesert3[4];
extern s16 ScrollingOverlay_SlotsDesert9[4];
extern s16 ScrollingOverlay_SlotsDesert10[8];
f32 Math_Floor(f32 x);

/* One sliding overlay of types 1-5: overlay 0 runs on Obj.var0 (its row), Obj.timer and Obj.var2
 * (its duration), overlay 1 on Obj.var1, Obj.var3 and Obj.var4; it moves 1000 along x from its row
 * over the duration, fading in over the first 20 frames and out over the last 20. */
#define OVERLAY_SLIDE_ROW(tbl, off)                                    \
    if (i == 0) {                                            \
        duration = obj->var2;                                       \
        elapsed = obj->timer;                                       \
        row = tbl[obj->var0];                                \
        w = elapsed * (1000.0f / duration);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    } else {                                                 \
        duration = obj->var4;                                    \
        elapsed = obj->var3;                                       \
        row = tbl[obj->var1];                                \
        w = elapsed * (1000.0f / duration);                               \
        x = w + row[0];                                      \
        y = row[1];                                          \
        z = row[2];                                          \
    }                                                        \
    if (elapsed < 20) {                                            \
        alpha = (u8) (255.0f * (0.05f * elapsed));                     \
    } else if (duration - 20 < elapsed) {                                 \
        alpha = (u8) (255.0f * ((f32) (duration - elapsed) / 20.0f));         \
    } else {                                                 \
        alpha = 0xFF;                                            \
    }                                                        \
    mdl = stage->matSets + off;                              \
    Gs_SetState(0x67, alpha << 24 | 0xFFFFFF);                   \
    break;

/*
 * Draws the type's overlays without depth writes: each enabled slot (its
 * ScrollingOverlay_Slots table, up to four) draws a model of the area's sub-
 * model with its texture scrolled: slots 0 and 1 by the area's frame count
 * (slower in AREA_DESERT_10 and AREA_JUNGLE_NIGHT_3), slot 2 through the 16
 * steps of ScrollingOverlay_FlipbookUV, slot 3 along u over 300 frames. Type
 * 0 draws models 1-3 at ScrollingOverlay_ForestHillsPos, type 6 model 2 at
 * the origin, types 1-5 their one model sliding along its row as
 * OVERLAY_SLIDE_ROW says.
 */
void ScrollingOverlay_Draw(DrawSlot *drawSlot)
{
    f32 posMtx[16];
    f32 uvMtx[16];
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *areaState = &gAreaState;
    ModelMatSet *mdl;
    s16 *slotOn;
    f32 *pos;
    s32 off;
    s16 i;
    f32 texU;
    f32 texV;
    f32 z;
    f32 x;
    f32 y;
    f32 *row;
    f32 w;
    s16 elapsed;
    s16 duration;
    u8 alpha;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        switch (obj->type) {
        case 0: slotOn = ScrollingOverlay_SlotsForestHills; break;
        case 1: slotOn = ScrollingOverlay_SlotsDesertNight7; break;
        case 2: slotOn = ScrollingOverlay_SlotsGreatArena; break;
        case 3: slotOn = ScrollingOverlay_SlotsDesert2; break;
        case 4: slotOn = ScrollingOverlay_SlotsDesert3; break;
        case 5: slotOn = ScrollingOverlay_SlotsDesert9; break;
        case 6: slotOn = ScrollingOverlay_SlotsDesert10; break;
        default: return;
        }
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x60, 0);
        pos = ScrollingOverlay_ForestHillsPos[0];
        for (i = 0, off = 0; i < 4; i++) {
            if (*slotOn++ != 0) {
                texU = 0.0f;
                texV = texU;
                switch (i) {
                case 0:
                    if (gScene.area == AREA_DESERT_10) {
                        texV = 1.0f - areaState->frame / 100.0f;
                    } else {
                        texV = 1.0f - 0.011111111f * areaState->frame;
                    }
                    break;
                case 1:
                    if (gScene.area == AREA_JUNGLE_NIGHT_3) {
                        texV = 1.0f - 0.015625f * areaState->frame;
                    } else {
                        texV = 1.0f - 0.03125f * areaState->frame;
                    }
                    break;
                case 2:
                    texU = ScrollingOverlay_FlipbookUV[(obj->timer & 0x1E) >> 1][0];
                    texV = ScrollingOverlay_FlipbookUV[(obj->timer & 0x1E) >> 1][1];
                    break;
                case 3:
                    texU = 0.0033333334f * ((u16) obj->timer % 300);
                    break;
                }
                texU -= Math_Floor(texU);
                texV -= Math_Floor(texV);
                Mtx_Translation(uvMtx, texU, texV, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                x = 0.0f;
                y = x;
                z = x;
                switch (obj->type) {
                case 0:
                    x = pos[0];
                    y = pos[1];
                    z = pos[2];
                    mdl = (ModelMatSet *) ((u8 *) stage->matSets + off + 0x8C);
                    break;
                case 1:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesertNight7, 0)
                case 2:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsGreatArena, 5)
                case 3:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert2, 3)
                case 4:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert3, 7)
                case 5:
                    OVERLAY_SLIDE_ROW(ScrollingOverlay_RowsDesert9, 3)
                case 6:
                    mdl = stage->matSets + 2;
                    break;
                }
                Mtx_Translation(posMtx, x, y, z);
                Gs_SetState(0x1A, (s32) posMtx);
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

/*
 * Creates the five TurningProp objects (kind 0x10, types 0-4, no buffer) in
 * AREA_BATTLE_TOWN_0 and its night, where Area_StartObjects calls it; creates
 * nothing elsewhere.
 */
void TurningProp_Create(void)
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
                obj->update = TurningProp_Update;
            }
        }
        break;
    }
}

/*
 * TurningProp's update, run every frame: calls the set-up, run, end or free
 * function by Obj.state (0-3).
 */
void TurningProp_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        TurningProp_Setup(obj);
        break;
    case 1:
        TurningProp_Run(obj);
        break;
    case 2:
        TurningProp_End(obj);
        break;
    case 3:
        TurningProp_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the prop at TurningProp_Pos[type], picks its turn
 * period of 200 to 327 frames (Obj.var3), takes a draw slot
 * (TurningProp_Draw) and runs the first frame, or frees the object when none
 * is free.
 */
void TurningProp_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->pos[0] = 0.0f;
    obj->pos[1] = 0.0f;
    obj->pos[2] = 0.0f;
    obj->timer = 0;
    Vec_Copy(obj->pos, TurningProp_Pos[obj->type]);
    obj->var3 = (Rand_Next(1) & 0x7F) + 200;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = TurningProp_Draw;
        TurningProp_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: counts Obj.timer through the turn period and adds the draw slot
 * to gDrawOt32.
 */
void TurningProp_Run(Obj *obj)
{
    obj->timer++;
    if (obj->timer >= obj->var3) {
        obj->timer = 0;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/* End state: hides the prop; its draw slot is not freed. */
void TurningProp_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void TurningProp_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the prop: model TurningProp_Models[type] of the area model with its
 * lights, scaled by TurningProp_Scales[type] and turned about y once per
 * period (the angle falls as Obj.timer grows) at the prop's position, alpha
 * test reference 0x80.
 */
void TurningProp_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaState.model;
    f32 mtx[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = &stage->matSets[TurningProp_Models[obj->type]];
        Gs_SetLightList(stage->mats, model);
        Mtx_Scale(mtx, TurningProp_Scales[obj->type], TurningProp_Scales[obj->type], TurningProp_Scales[obj->type]);
        Mtx_RotateY(mtx, (6.2831855f * (360.0f * (1.0f - (f32) obj->timer / (f32) obj->var3))) / 360.0f);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x60, 0x80);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the GroundTileGrid object (kind 0x11, no buffer); Area_StartObjects
 * calls it for AREA_JUNGLE_1, AREA_JUNGLE_6 and their nights.
 */
void GroundTileGrid_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x11;
        obj->type = 0;
        obj->update = GroundTileGrid_Update;
    }
}

/*
 * GroundTileGrid's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void GroundTileGrid_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        GroundTileGrid_Setup(obj);
        break;
    case 1:
        GroundTileGrid_Run(obj);
        break;
    case 2:
        GroundTileGrid_End(obj);
        break;
    case 3:
        GroundTileGrid_Free(obj);
        break;
    }
}

/*
 * Set-up state: zeroes its position and counters, takes a draw slot
 * (GroundTileGrid_Draw) and runs the first frame, or frees the object when
 * none is free.
 */
void GroundTileGrid_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->pos[0] = 0.0f;
    obj->pos[1] = 0.0f;
    obj->pos[2] = 0.0f;
    obj->timer = 0;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = GroundTileGrid_Draw;
        GroundTileGrid_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: follows the local player on x and z at height 0 (the slot's
 * depth) and adds the draw slot to gDrawOt8; returns OT_AddByDepth's result.
 */
u32 GroundTileGrid_Run(Obj *obj)
{
    Player *pl;

    pl = &gPlayers[gScene.localPlayer];
    obj->timer++;
    obj->pos[0] = pl->pos[0];
    obj->pos[1] = 0.0f;
    obj->pos[2] = pl->pos[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    return OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
}

/* End state: hides the object; its draw slot is not freed. */
void GroundTileGrid_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void GroundTileGrid_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the area's tile grid (by Area_DayId): AREA_JUNGLE_1 a 5 by 5 grid of
 * 2000-unit tiles (GroundTileGrid_Jungle1Tiles), AREA_JUNGLE_6 18 rows of 7
 * tiles of 500 by 1000 (GroundTileGrid_Jungle6Tiles), each turned by
 * GroundTileGrid_Jungle6Turns. A tile (its low 4 bits: its model of the
 * area's sub-model; 0xFF: none) is drawn at height 0 when its bounding sphere
 * is in the view frustum (Camera_BuildFrustum out to 4000). In the first case
 * the assignments stay in this order: the compiler then loads the table's
 * address after the radius is stored, as the game's code does.
 */
void GroundTileGrid_Draw(DrawSlot *drawSlot)
{
    f32 out[4];
    f32 mtx[16];
    f32 sph[8];
    s32 k;
    ModelMatSet *mdl;
    u16 day;
    s16 i;
    s16 j;
    s16 rows;
    Model *stage = gAreaSubModels[0];
    u8 tileId;
    s16 cols;
    u8 *tbl;
    f32 startX;
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
            startX = 8000.0f;
            zstep = 2000.0f;
            sph[4] = 2828.0f;
            xstep = 2000.0f;
            rows = 5;
            cols = 5;
            tbl = GroundTileGrid_Jungle1Tiles;
            Camera_BuildFrustum(4000.0f);
            break;
        case 0x74:
            z = 5800.0f;
            startX = 10100.0f;
            rows = 18;
            cols = 7;
            xstep = 500.0f;
            zstep = 1000.0f;
            sph[4] = 1414.0f;
            tbl = GroundTileGrid_Jungle6Tiles;
            Camera_BuildFrustum(4000.0f);
            break;
        }
        sph[2] = 0.0f;
        Tex_Prepare(0x10, 0x128);
        for (i = 0; i < rows; i++) {
            x = startX;
            for (j = 0; j < cols; j++) {
                k = i * cols + j;
                tileId = tbl[(s16) k];
                if (tileId != 0xFF) {
                    sph[1] = x;
                    sph[3] = z;
                    if (Frustum_TestSphere(&sph[1], out, gViewMatrix, gFrustumPlanes, sph[4]) != 0) {
                        mdl = stage->matSets + (tileId & 0xF);
                        Mtx_Translation(mtx, x, 0.0f, z);
                        if (day == 0x74) {
                            Mtx_RotateY(mtx, (6.2831855f * GroundTileGrid_Jungle6Turns[k]) / 65536.0f);
                        }
                        Gs_SetState(0x1A, (s32) mtx);
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

/*
 * Creates the AnimatedScenery object (kind 0x13, no buffer) of scenery set
 * setId (its Obj.type); Area_StartObjects passes the area's third set-up id
 * of gAreaInfo.
 */
void AnimatedScenery_Create(u8 setId)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x13;
        obj->type = setId;
        obj->update = AnimatedScenery_Update;
    }
}

/*
 * AnimatedScenery's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void AnimatedScenery_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedScenery_Setup(obj);
        break;
    case 1:
        AnimatedScenery_Run(obj);
        break;
    case 2:
        AnimatedScenery_End(obj);
        break;
    case 3:
        AnimatedScenery_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the object at its set's first position
 * (AnimatedScenery_Positions[type]), clears its counters, takes a draw slot
 * (AnimatedScenery_Draw) at that position and runs the first frame, or frees
 * the object when none is free.
 */
void AnimatedScenery_Setup(Obj *obj)
{
    f32 *posRow;

    posRow = AnimatedScenery_Positions[obj->type];
    obj->state++;
    obj->visible = 1;
    obj->pos[0] = *posRow++;
    obj->pos[1] = *posRow++;
    obj->pos[2] = *posRow;
    obj->timer = 0;
    obj->var3 = 0;
    obj->var5 = 0;
    obj->fvar = 0.0f;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AnimatedScenery_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AnimatedScenery_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: counts Obj.timer up and the set's own counter Obj.var3: wrapping
 * at 896 (set 0x19), 600 (set 0x24, whose Obj.var5 wraps at 1200) or 800
 * (sets 0x36, 0x37), or stepping as an angle by 0x2E (set 0x25) or 0x5B (the
 * others) a frame. Adds the draw slot to gDrawOt8 and returns OT_AddByDepth's
 * result.
 */
u32 AnimatedScenery_Run(Obj *obj)
{

    obj->timer++;
    switch (obj->type) {
    case 0x19:
        obj->var3++;
        if (obj->var3 >= 0x380) {
            obj->var3 = 0;
        }
        break;
    case 0x24:
        obj->var5++;
        if (obj->var5 >= 0x4B0) {
            obj->var5 = 0;
        }
        obj->var3++;
        if (obj->var3 >= 0x258) {
            obj->var3 = 0;
        }
        break;
    case 0x25:
        obj->var3 += 0x2E;
        break;
    case 0x36:
        obj->var3++;
        if (obj->var3 >= 0x320) {
            obj->var3 = 0;
        }
        break;
    case 0x37:
        obj->var3++;
        if (obj->var3 >= 0x320) {
            obj->var3 = 0;
        }
        break;
    default:
        obj->var3 += 0x5B;
        break;
    }
    return OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
}

/* End state: hides the object; its draw slot is not freed. */
void AnimatedScenery_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Free state: Obj_Free. */
void AnimatedScenery_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Returns 1 for the scenery sets drawn from the area model with its lights
 * (0x20-0x22, 0x36, 0x37), else 0. obj is the AnimatedScenery object, passed
 * as an Em (Em.subType lies where Obj.type does); type is unused.
 */
s32 func_006FD010(Em *obj, u8 type)
{
    switch (obj->subType) {
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x36:
    case 0x37:
        return 1;
    }
    return 0;
}

extern s16 AnimatedScenery_FirstModel[56];
extern s16 *AnimatedScenery_Hidden[56];
extern f32 AnimatedScenery_FlipbookUV[16][2];

/* The models of area object K13 (by type): type 0x11 first draws the other stage's model 8. Then up
 * to four models (the 0 entries of AnimatedScenery_Hidden[type]; AnimatedScenery_FirstModel[type] the first model, AnimatedScenery_Positions[type]
 * the positions), each with a texture scroll by type (unk008, unk00A; AnimatedScenery_FlipbookUV a 4x4 tile grid).
 * Level 4 (the 8 FPU register words of the two 0.0333f * 360 * unk00A lines differ at 3): the functions
 * GroundTileGrid_Create-CastleGate_Draw all match at 4, DistantLightning_Draw differs there. */
#pragma optimization_level 4
/*
 * Draws the set's models: up to four slots, each drawn when its
 * AnimatedScenery_Hidden[type] entry is 0, with the next model from
 * AnimatedScenery_FirstModel[type] on and the next position of
 * AnimatedScenery_Positions[type], its texture scrolled by the set
 * (Obj.timer, Obj.var3 or Obj.var5; AnimatedScenery_FlipbookUV for the
 * 16-step sets); the first slot of the sets not listed turns about y by
 * Obj.var3. Set 0x11 first draws model 8 of the area model; the sets
 * AnimatedScenery_UsesAreaModel accepts draw from the area model with its
 * lights. The pragma compiles it at optimization level 4: at the file's level
 * the compiler allocates the floating-point registers of the two angle-scroll
 * lines differently from the game's code.
 */
void AnimatedScenery_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    AreaState *areaState = &gAreaState;
    ModelMat *lights;
    ModelMatSet *mdl;
    s16 k;
    s16 *flags;
    f32 *pos;
    f32 x;
    f32 y;
    f32 texU;
    f32 texV;

    if (obj->type == 0x11) {
        stage = areaState->model;
        lights = stage->mats;
        if (stage == NULL) {
            return;
        }
        if (stage->active == 0) {
            return;
        }
        Mtx_Identity(mtx);
        mdl = stage->matSets + 8;
        Gs_SetLightList(lights, mdl);
        Gs_SetState(0x1A, (s32) mtx);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
        stage = gAreaSubModels[0];
    }
    if (func_006FD010((Em *) obj, obj->type)) {
        stage = areaState->model;
        lights = stage->mats;
    }
    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x67, -1);
        pos = AnimatedScenery_Positions[obj->type];
        flags = AnimatedScenery_Hidden[obj->type];
        k = AnimatedScenery_FirstModel[obj->type];
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = 0.0f;
            y = x;
            switch (obj->type) {
            case 0x14:
            case 0x2C:
                Gs_SetState(0x60, 0);
                x = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 0x1B:
            case 0x1F:
            case 0x34:
                Gs_SetState(0x60, 0);
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x10:
                Gs_SetState(0x60, 0);
                break;
            case 0xC:
                Gs_SetState(0x60, 0);
                x = 0.125f * (obj->timer & 7);
                y = 0.125f * ((obj->timer >> 3) & 3);
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
                x = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            case 0xB:
                Gs_SetState(0x60, 0);
                y = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            default:
                if (obj->type == 0x25 || obj->type == 0x29) {
                    Gs_SetState(0x60, 0);
                } else {
                    Gs_SetState(0x60, 0x80);
                }
                x = 0.033333335f * ((360.0f * obj->var3) / 65536.0f);
                x -= Math_Floor(x);
                Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
                break;
            }
            Mtx_Translation(uvMtx, x, y, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(0x1A, (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(0x60, 0);
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            switch (obj->type) {
            case 0:
            case 1:
            case 0x14:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x1D:
            case 0x2B:
            case 0x2C:
                Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0x31:
                Gs_SetState(0x6C, 0);
                Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0xC:
            case 0x1B:
                Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0x7F) / 128.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 8:
            case 0x28:
                Mtx_Translation(uvMtx, 0.0f, 1.0f - (obj->timer & 0x1FF) / 512.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0xB:
                Mtx_Translation(uvMtx, 1.0f - (obj->timer & 0xF) / 16.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0x1E:
                Mtx_Translation(uvMtx, 0.125f * (obj->timer & 7), 0.125f * ((obj->timer >> 3) & 3), 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0x1F:
            case 0x34:
                Mtx_Translation(uvMtx, (obj->timer & 0xFF) / 256.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0x24:
                Mtx_Translation(uvMtx, (obj->timer & 0x7FF) / 2048.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 0x10:
            case 0x26:
            case 0x29:
                break;
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(0x1A, (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x6C, 1);
        }
        if (*flags++ == 0) {
            Mtx_Identity(mtx);
            texU = 0.0f;
            texV = texU;
            switch (obj->type) {
            case 4:
            case 7:
                texV = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0xC:
            case 0x14:
            case 0x2A:
            case 0x2B:
            case 0x2E:
                texU = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 0x13:
                texU = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x10:
            case 0x1B:
                break;
            case 0x19:
                texV = obj->var3 / 896.0f;
                texU = texV;
                break;
            case 0x1C:
                texV = (obj->timer & 0x7F) / 128.0f;
                break;
            case 1:
            case 0xD:
            case 0x1F:
            case 0x2C:
                texU = 0.125f * (obj->timer & 7);
                texV = 0.125f * ((obj->timer >> 3) & 3);
                break;
            case 0x1E:
                texU = (obj->timer & 0x3F) / 64.0f;
                break;
            case 0x25:
                texU = 0.033333335f * ((360.0f * obj->var3) / 65536.0f);
                texU -= Math_Floor(texU);
                Mtx_RotateY(mtx, (6.2831855f * obj->var3) / 65536.0f);
                texV = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x12:
            case 0x1A:
            case 0x23:
            case 0x27:
            case 0x30:
                texU = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 8:
                texU = (obj->timer & 0x1FF) / 512.0f;
                texV = 1.0f - (obj->timer & 0x7F) / 128.0f;
                break;
            case 0x28:
                Gs_SetState(0x6C, 0);
                texU = (obj->timer & 0x1FF) / 512.0f;
                texV = 1.0f - (obj->timer & 0x7F) / 128.0f;
                break;
            case 0x24:
                texU = obj->var5 / 1200.0f;
                break;
            case 9:
            case 0x26:
            case 0x29:
                texV = (obj->timer & 0x7FF) / 2048.0f;
                break;
            case 0x21:
            case 0x22:
                texU = 1.0f - (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 0x2D:
                texV = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 0x33:
                texV = 1.0f - (obj->timer & 0x1FF) / 512.0f;
                break;
            case 0xB:
            case 0x34:
                break;
            case 0x36:
            case 0x37:
                texU = (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 0x20:
                texV = (obj->timer & 0x3FF) / 1024.0f;
                break;
            default:
                texU = AnimatedScenery_FlipbookUV[obj->timer & 0xF][0];
                texV = AnimatedScenery_FlipbookUV[obj->timer & 0xF][1];
                break;
            }
            Mtx_Translation(uvMtx, texU, texV, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(0x1A, (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Gs_SetState(0x6C, 1);
        if (*flags++ == 0) {
            mdl = stage->matSets + k++;
            if (func_006FD010((Em *) obj, obj->type)) {
                Gs_SetLightList(lights, mdl);
            }
            Mtx_Identity(mtx);
            switch (obj->type) {
            case 2:
            case 3:
            case 5:
            case 6:
            case 0xF:
            case 0x20:
            case 0x21:
                y = 0.0f;
                x = (obj->timer & 0x3FF) / 1024.0f;
                break;
            case 8:
            case 0x15:
            case 0x28:
                y = 0.0f;
                x = (obj->timer & 0x7F) / 128.0f;
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
                x = 0.125f * (obj->timer & 7);
                y = 0.125f * ((obj->timer >> 3) & 3);
                break;
            case 0xB:
                Gs_SetState(0x6C, 0);
                y = 0.0f;
                x = (obj->timer & 0x7F) / 128.0f;
                break;
            case 0x1F:
                y = 0.0f;
                x = (obj->timer & 0x3F) / 64.0f;
                break;
            case 0x1E:
                y = 0.0f;
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x33:
                Gs_SetState(0x6C, 0);
                y = 0.0f;
                x = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x11:
            case 0x1A:
            case 0x1B:
                y = 0.0f;
                x = (obj->timer & 0x1FF) / 512.0f;
                break;
            case 1:
            case 0xD:
            case 0x2C:
                x = 0.0f;
                y = (obj->timer & 0x7F) / 128.0f;
                break;
            case 0x22:
                x = 0.0f;
                y = (obj->timer & 0x7FF) / 2048.0f;
                break;
            case 0:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x1D:
            case 0x30:
                x = 0.0f;
                y = (obj->timer & 0xFF) / 256.0f;
                break;
            case 0x24:
                y = 0.0f;
                x = obj->var3 / 600.0f;
                break;
            case 0x36:
                y = 0.0f;
                x = obj->var3 / 800.0f;
                break;
            case 0x37:
                y = 0.0f;
                x = obj->var3 / 800.0f;
                Gs_SetState(0x67, 0xFF303030);
                break;
            case 0xA:
            case 0x32:
                x = 0.0f;
                y = x;
                break;
            }
            Mtx_Translation(uvMtx, x, y, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            x = *pos++;
            y = *pos++;
            Mtx_SetTrans(mtx, x, y, *pos++);
            Gs_SetState(0x1A, (s32) mtx);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
            Gs_SetState(0x6C, 1);
        }
        Draw_SetDefaultState();
    }
}
#pragma optimization_level reset

/*
 * Creates the CastleGate object of gate gateType (0: AREA_CASTLE_0's, 1:
 * AREA_CASTLE_1's dropping gate). Unless gQuest.stage is 0 it first sets
 * gScene.skipFlaggedTris, the gate open (the collisions skip the triangles of
 * attribute 0x4000), in every quest but 0xBBC, which clears it.
 */
void CastleGate_Create(u8 gateType)
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
        obj->type = gateType;
        obj->update = CastleGate_Update;
    }
}

/*
 * CastleGate's update, run every frame: calls the set-up, run, end or free
 * function by Obj.state (0-3).
 */
void CastleGate_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        CastleGate_Setup(obj);
        break;
    case 1:
        CastleGate_Run(obj);
        break;
    case 2:
        CastleGate_End(obj);
        break;
    case 3:
        CastleGate_Free(obj);
        break;
    }
}

/*
 * Set-up state: places the gate at its open (CastleGate_PosOpen) or shut
 * (CastleGate_PosShut) position by gScene.skipFlaggedTris. When bit 1 of
 * gScene.emEventFlags is set (a Fatalis was knocked down or killed), it drops
 * at once (Obj.var0 2) if a Fatalis of the area lies knocked down, else never
 * drops (Obj.var0 4). Takes a draw slot (CastleGate_Draw) and runs the first
 * frame, or frees the object when none is free.
 */
void CastleGate_Setup(Obj *obj)
{
    s16 i;
    Em *em;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->fvar = 0.0f;
    if (gScene.skipFlaggedTris != 0) {
        Vec_Copy(obj->pos, CastleGate_PosOpen[obj->type]);
    } else {
        Vec_Copy(obj->pos, CastleGate_PosShut[obj->type]);
    }
    if (gScene.emEventFlags & 2) {
        em = gEms;
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
                    break;
                }
            }
        }
        if (i == 20 || em->motionId[0] != 0x403) {
            obj->var0 = 4;
        } else {
            obj->var0 = 2;
        }
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = CastleGate_Draw;
        CastleGate_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state of gate 1 while gScene.skipFlaggedTris is set, once: waits for a
 * Fatalis of the area standing on the ground under the gate (x 10900-13000, z
 * 19700-20200), lying knocked down, or after bit 1 of gScene.emEventFlags;
 * then plays its drop sounds, makes the hit under the gate
 * (StageShot_CreateAt, STAGESHOT_AREA_DROP, at the monster's floor) and drops
 * 1750 below its open height under gravity, shaking the camera and bouncing
 * up to three times. After 180 frames, once no Fatalis lies knocked down, it
 * plays the rise sounds and rises back at 29.2 a frame. Every frame adds the
 * draw slot to gDrawOt32.
 */
void CastleGate_Run(Obj *obj)
{
    Em *em = gEms;
    s16 i;
    f32 hitPos[3];

    switch (obj->type) {
    case 1:
        if (gScene.skipFlaggedTris == 0) {
            break;
        }
        switch (obj->var0) {
        case 0:
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                    if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
                        break;
                    }
                }
            }
            if (i == 20) {
                break;
            }
            if (!(gScene.emEventFlags & 2) && em->motionId[0] != 0x403) {
                if (em->stance != 0) {
                    break;
                }
                if (!(em->posX > 10900.0f && em->posX < 13000.0f && em->posZ > 19700.0f && em->posZ < 20200.0f)) {
                    break;
                }
            }
            Em_PlaySeChecked(em, 0x36, 0, CastleGate_SoundPos, 9, 0);
            Em_PlaySeChecked(em, 0x37, 0, D_008B149C, 9, 0);
            obj->var0++;
            obj->fvar = -100.0f;
            hitPos[0] = obj->pos[0];
            hitPos[1] = em->floorY;
            hitPos[2] = obj->pos[2];
            StageShot_CreateAt(hitPos, STAGESHOT_AREA_DROP, em->area, 0);
            break;
        case 1:
            obj->pos[1] += obj->fvar;
            obj->fvar += -20.0f;
            if (obj->pos[1] <= CastleGate_PosOpen[obj->type][1] - 1750.0f) {
                obj->pos[1] = CastleGate_PosOpen[obj->type][1] - 1750.0f;
                obj->fvar *= -0.2f;
                obj->var0++;
                CamShake_SetA(5, obj->pos);
                obj->timer = 0;
            }
            break;
        case 2:
            if (obj->var1 < 3) {
                obj->pos[1] += obj->fvar;
                obj->fvar += -20.0f;
                if (obj->pos[1] <= CastleGate_PosOpen[obj->type][1] - 1750.0f) {
                    obj->pos[1] = CastleGate_PosOpen[obj->type][1] - 1750.0f;
                    obj->fvar *= -0.2f;
                    obj->var1++;
                }
            }
            for (i = 0; i < 20; i++, em++) {
                if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                    if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
                        break;
                    }
                }
            }
            if (++obj->timer <= 180) {
                break;
            }
            if (i == 20 || em->motionId[0] != 0x403) {
                obj->var0++;
                obj->timer = 0;
                for (i = 0; i < 20; i++, em++) {
                    if (em->active != 0 && em->visible != 0 && Player_InCurrentArea(em) != 0) {
                        if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
                            Em_PlaySeChecked(em, 0x38, 0, CastleGate_SoundPos, 9, 0);
                            Em_PlaySeChecked(em, 0x39, 0, D_008B149C, 9, 0);
                            break;
                        }
                    }
                }
            }
            break;
        case 3:
            obj->pos[1] += 29.166666f;
            if (obj->pos[1] > CastleGate_PosOpen[obj->type][1]) {
                obj->var0++;
                obj->pos[1] = CastleGate_PosOpen[obj->type][1];
                obj->timer = 0;
            }
            break;
        case 4:
            break;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/* End state: hides the gate and frees its draw slot. */
void CastleGate_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Free state: Obj_Free. */
void CastleGate_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws model CastleGate_Models[type] of the area's sub-model at the gate's
 * position; the owner's Obj.pos and Obj.type are read through byte offsets.
 */
void CastleGate_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    s32 gate;
    Model *subs;
    ModelMatSet *model;

    subs = gAreaSubModels[0];
    gate = (s32) drawSlot->owner;
    if ((subs != NULL) && (subs->active != 0)) {
        Mtx_Translation((void *) mtx, *(f32 *)((u8 *)gate + 0x20), *(f32 *)((u8 *)gate + 0x24), *(f32 *)((u8 *)gate + 0x28));
        Gs_SetState(0x1A, (s32) mtx);
        model = &subs->matSets[CastleGate_Models[*(u8 *)((u8 *)gate + 3)]];
        if ((model != NULL) && (model->id != -1)) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the DistantLightning object (kind 0x16, no buffer);
 * Area_StartObjects calls it for AREA_CASTLE_1.
 */
void DistantLightning_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x16;
        obj->update = DistantLightning_Update;
    }
}

/*
 * DistantLightning's update, run every frame: calls the set-up, run, end or
 * free function by Obj.state (0-3).
 */
void DistantLightning_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        DistantLightning_Setup(obj);
        break;
    case 1:
        DistantLightning_Run(obj);
        break;
    case 2:
        DistantLightning_End(obj);
        break;
    case 3:
        DistantLightning_Free(obj);
        break;
    }
}

/*
 * Set-up state: clears its counters, waits 0 to 15 frames (Obj.timer), takes
 * a draw slot (DistantLightning_Draw) and runs the first frame, or frees the
 * object when none is free.
 */
void DistantLightning_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = Rand_Next(1) & 0xF;
    obj->var3 = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = DistantLightning_Draw;
        DistantLightning_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Run state: after its wait, a flash: picks a kind (Obj.var1, 0-4) and places
 * it 21700 from the point (10000, 10000) in a random direction, at height 0
 * (kinds 0, 1) or 6000 to 9300 up, with a random roll (Obj.var3) and, for
 * kind 4, a scale of 0.7 to 1.0 (Obj.fvar); shows it 6 frames through
 * gDrawOt8, then waits 1 to 64 frames. Kind 5, which Rand_Next(1) % 5 never
 * gives, would also spawn four StageObj_K11 effects and draw through
 * gDrawOtSorted. y stays a variable set to 0.0f: the game's code adds it to
 * the height.
 */
void DistantLightning_Run(Obj *obj)
{
    f32 offset[3];
    f32 y = 0.0f;

    switch (obj->var0) {
    case 0:
        if (--obj->timer >= 0) {
            break;
        }
        obj->var0++;
        obj->var1 = Rand_Next(1) % 5;
        obj->timer = 7;
        offset[0] = 0.0f;
        offset[2] = 21700.0f;
        switch (obj->var1) {
        case 0:
        case 1:
        case 5:
            offset[1] = 0.0f;
            obj->var3 = (Rand_Next(1) & 0x7FF) - 0x400;
            obj->fvar = 1.0f;
            break;
        case 2:
            offset[1] = 7300.0f + (Rand_Next(1) & 0x3FF);
            obj->var3 = (Rand_Next(1) & 0xFFF) - 0x800;
            obj->fvar = 1.0f;
            break;
        case 3:
            offset[1] = 6000.0f + (Rand_Next(1) & 0x3FF) * 3;
            obj->var3 = (Rand_Next(1) & 0xFFF) - 0x800;
            obj->fvar = 1.0f;
            break;
        case 4:
            offset[1] = 7000.0f + (Rand_Next(1) & 0x7FF);
            obj->var3 = 0;
            obj->fvar = 0.7f + 0.0003f * (Rand_Next(1) & 0x3FF);
            break;
        }
        Vec_RotateY(offset, (6.2831855f * Rand_Next(1)) / 65536.0f);
        obj->pos[0] = 10000.0f + offset[0];
        obj->pos[1] = y + offset[1];
        obj->pos[2] = 10000.0f + offset[2];
        if (obj->var1 == 5) {
            AttackBurst_CreateAtScaled(obj->pos, 0, 4, 0.15f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 9, 1.0f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 8, 1.0f);
            AttackBurst_CreateAtScaled(obj->pos, 0, 0xB, 1.0f);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
            obj->timer = (Rand_Next(1) & 0x3F) + 1;
            break;
        }
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        if (obj->var1 == 5) {
            OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
        } else {
            OT_AddByDepth(gDrawOt8, obj->ds, 8, 0);
        }
        break;
    }
}

/* End state: hides the object and frees its draw slot. */
void DistantLightning_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Free state: Obj_Free. */
void DistantLightning_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the flash facing the camera: model pair Obj.var1 of the area's sub-
 * model (from model 3 on; the first for its first frame, the second after),
 * scaled by Obj.fvar, rolled about z by Obj.var3 and fading out over its last
 * 3 frames (alpha from Obj.timer), alpha test reference 0. The owner's fields
 * are read through byte offsets.
 */
void DistantLightning_Draw(DrawSlot *drawSlot)
{
    u8 *flash = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    f32 mtx[16];
    ModelMatSet *mdl;
    s32 alpha;
    s32 off;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x60, 0);
        Mtx_Scale(mtx, *(f32 *) (flash + 0x30), *(f32 *) (flash + 0x30), *(f32 *) (flash + 0x30));
        Mtx_RotateZ(mtx, (6.2831855f * *(s16 *) (flash + 0xA)) / 65536.0f);
        Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        if (*(s16 *) (flash + 8) < 4) {
            alpha = (u32) (255.0f * (*(s16 *) (flash + 8) / 4.0f));
        } else {
            alpha = 0xFF;
        }
        Gs_SetState(0x67, ((u8) alpha << 24) | 0xFFFFFF);
        Gs_SetState(0x1A, (s32) mtx);
        /* The offset is computed in bytes and added before the table's address, as the game's code
         * does. */
        mdl = stage->matSets;
        off = flash[6] * 0x118;
        mdl = (ModelMatSet *) (off + (s32) mdl + 0x1A4);
        if (*(s16 *) (flash + 8) < 6) {
            mdl++;
        }
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* The file's data. */
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
f32 AmbientLife_GlowBugTowerPos[32] = {
#include "game/008B0A10.inc"
};
f32 AmbientLife_CrossingFlyerPoints[6] = {
#include "game/008B0A90.inc"
};
f32 FortressTremor_PosX = {
#include "game/008B0AA8.inc"
};
f32 FortressTremor_PosZ = {
#include "game/008B0AB0.inc"
};
f32 FortressTremor_ImpactOffsets[12] = {
#include "game/008B0AC0.inc"
};
f32 BreakableStructure_Pos[5][3] = {
#include "game/008B0AF0.inc"
};
s16 BreakableStructure_ModelsDay[8] = {
#include "game/008B0B30.inc"
};
u16 BreakableStructure_Yaw[8] = {
#include "game/008B0B40.inc"
};
f32 BreakableStructure_Rise[6] = {
#include "game/008B0B50.inc"
};
s16 BreakableStructure_Tilt[6] = {
#include "game/008B0B68.inc"
};
f32 BreakableStructure_SoundPos[8] = {
#include "game/008B0B80.inc"
};
f32 BreakableStructure_EffectPos[3][3] = {
#include "game/008B0BA0.inc"
};
s16 BreakableStructure_ModelsNight[6] = {
#include "game/008B0BC8.inc"
};
f32 ScrollingOverlay_FlipbookUV[16][2] = {
#include "game/008B0BE0.inc"
};
f32 ScrollingOverlay_ForestHillsPos[4][3] = {
#include "game/008B0C60.inc"
};
f32 ScrollingOverlay_RowsDesertNight7[5][3] = {
#include "game/008B0C90.inc"
};
f32 ScrollingOverlay_RowsGreatArena[5][3] = {
#include "game/008B0CD0.inc"
};
f32 ScrollingOverlay_RowsDesert2[4][3] = {
#include "game/008B0D10.inc"
};
f32 ScrollingOverlay_RowsDesert3[4][3] = {
#include "game/008B0D40.inc"
};
f32 ScrollingOverlay_RowsDesert9[4][3] = {
#include "game/008B0D70.inc"
};
s16 ScrollingOverlay_SlotsForestHills[4] = {
#include "game/008B0DA0.inc"
};
s16 ScrollingOverlay_SlotsDesertNight7[4] = {
#include "game/008B0DA8.inc"
};
s16 ScrollingOverlay_SlotsGreatArena[4] = {
#include "game/008B0DB0.inc"
};
s16 ScrollingOverlay_SlotsDesert2[4] = {
#include "game/008B0DB8.inc"
};
s16 ScrollingOverlay_SlotsDesert3[4] = {
#include "game/008B0DC0.inc"
};
s16 ScrollingOverlay_SlotsDesert9[4] = {
#include "game/008B0DC8.inc"
};
s16 ScrollingOverlay_SlotsDesert10[8] = {
#include "game/008B0DD0.inc"
};
f32 TurningProp_Pos[5][3] = {
#include "game/008B0DE0.inc"
};
f32 TurningProp_Scales[6] = {
#include "game/008B0E20.inc"
};
s16 TurningProp_Models[6] = {
#include "game/008B0E38.inc"
};
u8 GroundTileGrid_Jungle1Tiles[32] = {
#include "game/008B0E50.inc"
};
u8 GroundTileGrid_Jungle6Tiles[80] = {
#include "game/008B0E70.inc"
};
u16 GroundTileGrid_Jungle6Turns[72] = {
#include "game/008B0EC0.inc"
};
u32 AnimatedScenery_Models_1[2] = {
#include "game/008B0F50.inc"
};
f32 AnimatedScenery_Params_2[3] = {
#include "game/008B0F58.inc"
};
u32 AnimatedScenery_Models_2[2] = {
#include "game/008B0F68.inc"
};
f32 AnimatedScenery_Params_3[4] = {
#include "game/008B0F70.inc"
};
u32 AnimatedScenery_Models_3[4] = {
#include "game/008B0F80.inc"
};
f32 AnimatedScenery_Params_4[10] = {
#include "game/008B0F90.inc"
};
u32 AnimatedScenery_Models_4[2] = {
#include "game/008B0FB8.inc"
};
f32 AnimatedScenery_Params_5[4] = {
#include "game/008B0FC0.inc"
};
u32 AnimatedScenery_Models_5[2] = {
#include "game/008B0FD0.inc"
};
f32 AnimatedScenery_Params_6[3] = {
#include "game/008B0FD8.inc"
};
u32 AnimatedScenery_Models_6[2] = {
#include "game/008B0FE8.inc"
};
f32 AnimatedScenery_Params_7[6] = {
#include "game/008B0FF0.inc"
};
u32 AnimatedScenery_Models_7[2] = {
#include "game/008B1008.inc"
};
u32 AnimatedScenery_Models_9[2] = {
#include "game/008B1010.inc"
};
u32 AnimatedScenery_Models_10[2] = {
#include "game/008B1018.inc"
};
u32 AnimatedScenery_Models_13[2] = {
#include "game/008B1020.inc"
};
u32 AnimatedScenery_Models_16[2] = {
#include "game/008B1028.inc"
};
u32 AnimatedScenery_Models_17[2] = {
#include "game/008B1030.inc"
};
u32 AnimatedScenery_Models_18[2] = {
#include "game/008B1038.inc"
};
u32 AnimatedScenery_Models_19[2] = {
#include "game/008B1040.inc"
};
u32 AnimatedScenery_Models_21[2] = {
#include "game/008B1048.inc"
};
u32 AnimatedScenery_Models_0[2] = {
#include "game/008B1050.inc"
};
f32 AnimatedScenery_Params_25[3] = {
#include "game/008B1058.inc"
};
u32 AnimatedScenery_Models_25[2] = {
#include "game/008B1068.inc"
};
u32 AnimatedScenery_Models_26[2] = {
#include "game/008B1070.inc"
};
u32 AnimatedScenery_Models_28[2] = {
#include "game/008B1078.inc"
};
u32 AnimatedScenery_Models_32[2] = {
#include "game/008B1080.inc"
};
u32 AnimatedScenery_Models_33[2] = {
#include "game/008B1088.inc"
};
u32 AnimatedScenery_Models_34[2] = {
#include "game/008B1090.inc"
};
u32 AnimatedScenery_Models_35[2] = {
#include "game/008B1098.inc"
};
u32 AnimatedScenery_Models_36[4] = {
#include "game/008B10A0.inc"
};
f32 AnimatedScenery_Params_37[10] = {
#include "game/008B10B0.inc"
};
u32 AnimatedScenery_Models_37[2] = {
#include "game/008B10D8.inc"
};
u32 AnimatedScenery_Models_38[2] = {
#include "game/008B10E0.inc"
};
u32 AnimatedScenery_Models_39[2] = {
#include "game/008B10E8.inc"
};
u32 AnimatedScenery_Models_8[4] = {
#include "game/008B10F0.inc"
};
f32 AnimatedScenery_Params_41[12] = {
#include "game/008B1100.inc"
};
u32 AnimatedScenery_Models_42[2] = {
#include "game/008B1130.inc"
};
u32 AnimatedScenery_Models_43[2] = {
#include "game/008B1138.inc"
};
f32 AnimatedScenery_Params_45[4] = {
#include "game/008B1140.inc"
};
u32 AnimatedScenery_Models_45[2] = {
#include "game/008B1150.inc"
};
u32 AnimatedScenery_Models_46[2] = {
#include "game/008B1158.inc"
};
u32 AnimatedScenery_Models_47[2] = {
#include "game/008B1160.inc"
};
u32 AnimatedScenery_Models_48[2] = {
#include "game/008B1168.inc"
};
u32 AnimatedScenery_Models_49[2] = {
#include "game/008B1170.inc"
};
u32 AnimatedScenery_Models_51[2] = {
#include "game/008B1178.inc"
};
u32 AnimatedScenery_Models_50[2] = {
#include "game/008B1180.inc"
};
u32 AnimatedScenery_Models_52[2] = {
#include "game/008B1188.inc"
};
u32 AnimatedScenery_Models_53[2] = {
#include "game/008B1190.inc"
};
u32 AnimatedScenery_Models_54[2] = {
#include "game/008B1198.inc"
};
f32 *AnimatedScenery_Positions[56] = {
    (f32 *)D_008F5440, (f32 *)D_008F5290, AnimatedScenery_Params_2, AnimatedScenery_Params_3,
    AnimatedScenery_Params_4, AnimatedScenery_Params_5, AnimatedScenery_Params_6, AnimatedScenery_Params_7,
    (f32 *)D_008F5720, (f32 *)D_008F52C0, (f32 *)D_008F52D8, (f32 *)D_008F52F0,
    (f32 *)D_008F5330, (f32 *)D_008F5370, AnimatedScenery_Params_2, AnimatedScenery_Params_6,
    (f32 *)D_008F5390, (f32 *)D_008F53B8, (f32 *)D_008F53C8, (f32 *)D_008F53D8,
    (f32 *)D_008F53F0, (f32 *)D_008F5428, (f32 *)D_008F5440, (f32 *)D_008F5470,
    (f32 *)D_008F54B0, AnimatedScenery_Params_25, (f32 *)D_008F54F0, (f32 *)D_008F5510,
    (f32 *)D_008F5548, (f32 *)D_008F5560, (f32 *)D_008F55A0, (f32 *)D_008F55E0,
    (f32 *)D_008F5620, (f32 *)D_008F5640, (f32 *)D_008F5660, (f32 *)D_008F5680,
    (f32 *)D_008F56A0, AnimatedScenery_Params_37, (f32 *)D_008F56D0, (f32 *)D_008F5700,
    (f32 *)D_008F5720, AnimatedScenery_Params_41, (f32 *)D_008F5750, (f32 *)D_008F5768,
    (f32 *)D_008F5780, AnimatedScenery_Params_45, (f32 *)D_008F57C0, (f32 *)D_008F57D8,
    (f32 *)D_008F57F0, (f32 *)D_008F5818, (f32 *)D_008F5848, (f32 *)D_008F5830,
    (f32 *)D_008F5860, (f32 *)D_008F5888, (f32 *)D_008F58A0, (f32 *)D_008F58A0,
};
s16 *AnimatedScenery_Hidden[56] = {
    (s16 *)AnimatedScenery_Models_0, (s16 *)AnimatedScenery_Models_1, (s16 *)AnimatedScenery_Models_2, (s16 *)AnimatedScenery_Models_3,
    (s16 *)AnimatedScenery_Models_4, (s16 *)AnimatedScenery_Models_5, (s16 *)AnimatedScenery_Models_6, (s16 *)AnimatedScenery_Models_7,
    (s16 *)AnimatedScenery_Models_8, (s16 *)AnimatedScenery_Models_9, (s16 *)AnimatedScenery_Models_10, (s16 *)D_008F5320,
    (s16 *)D_008F5360, (s16 *)AnimatedScenery_Models_13, (s16 *)AnimatedScenery_Models_2, (s16 *)AnimatedScenery_Models_6,
    (s16 *)AnimatedScenery_Models_16, (s16 *)AnimatedScenery_Models_17, (s16 *)AnimatedScenery_Models_18, (s16 *)AnimatedScenery_Models_19,
    (s16 *)D_008F5420, (s16 *)AnimatedScenery_Models_21, (s16 *)AnimatedScenery_Models_0, (s16 *)D_008F54A0,
    (s16 *)D_008F54E0, (s16 *)AnimatedScenery_Models_25, (s16 *)AnimatedScenery_Models_26, (s16 *)D_008F5540,
    (s16 *)AnimatedScenery_Models_28, (s16 *)D_008F5590, (s16 *)D_008F55D0, (s16 *)D_008F5610,
    (s16 *)AnimatedScenery_Models_32, (s16 *)AnimatedScenery_Models_33, (s16 *)AnimatedScenery_Models_34, (s16 *)AnimatedScenery_Models_35,
    (s16 *)AnimatedScenery_Models_36, (s16 *)AnimatedScenery_Models_37, (s16 *)AnimatedScenery_Models_38, (s16 *)AnimatedScenery_Models_39,
    (s16 *)AnimatedScenery_Models_8, (s16 *)D_008F5748, (s16 *)AnimatedScenery_Models_42, (s16 *)AnimatedScenery_Models_43,
    (s16 *)D_008F57B0, (s16 *)AnimatedScenery_Models_45, (s16 *)AnimatedScenery_Models_46, (s16 *)AnimatedScenery_Models_47,
    (s16 *)AnimatedScenery_Models_48, (s16 *)AnimatedScenery_Models_49, (s16 *)AnimatedScenery_Models_50, (s16 *)AnimatedScenery_Models_51,
    (s16 *)AnimatedScenery_Models_52, (s16 *)AnimatedScenery_Models_53, (s16 *)AnimatedScenery_Models_54, (s16 *)AnimatedScenery_Models_54,
};
s16 AnimatedScenery_FirstModel[56] = {
#include "game/008B1360.inc"
};
f32 AnimatedScenery_FlipbookUV[16][2] = {
#include "game/008B13D0.inc"
};
f32 CastleGate_PosOpen[2][3] = {
#include "game/008B1450.inc"
};
f32 CastleGate_PosShut[2][3] = {
#include "game/008B1470.inc"
};
s16 CastleGate_Models[4] = {
#include "game/008B1488.inc"
};
f32 CastleGate_SoundPos[6] = {
#include "game/008B1490.inc"
};
