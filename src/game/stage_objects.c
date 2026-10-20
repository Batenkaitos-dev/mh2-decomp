/*
 * Stage objects (StageObj, enum StageObjKind), nine families in the game's
 * order:
 * - ImpactRocks: rocks flung from the great sword's ground impact;
 * - DemonGlow: the glow on the dual swords' blades in demon mode;
 * - ThunderSpark: the Khezu's and the Kirin's electric sparks;
 * - WeaponTrail: the trail a blade leaves as it swings;
 * - EmBodyFx: a monster's glow, cover or prop drawn on its bones (the
 *   Kirin, the Shakalaka, monster kind 0x38, the Yama Tsukami);
 * - EmTail: a monster's severable tail, which falls and can be carved;
 * - LaoshanDust: the Lao-Shan Lung's dust clouds;
 * - PlayerHitBurst: a burst on a hunter who was hit;
 * - ItemProp: what an item leaves in the stage, drawn by type (the rest
 *   of the family is with the hit effects).
 * Each family has a creator, an update that switches on StageObj.state
 * (enum StageObjState: set-up, run, end, free) and a draw callback for its
 * draw slots. Their records (ImpactRock, WeaponTrailBuf ...) and their
 * types' names are in stageobj.h.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "quest.h"
#include "stageobj.h"
#include "player.h"
#include "em_rathalos.h"
#include "em_kushala.h"
#include "em_garuga.h"
#include "em_gravios.h"
#include "em.h"
#include "objhead.h"
#include "objattach.h"
#include "lobby_vobj.h"
#include "area.h"
/* The game built this file at optimization level 4. */
#pragma optimization_level 4
/* prototypes of callees */
u8 Player_InCurrentArea(void *obj);
struct EmHolder;
/* prototypes */
s32 EmHolder_CreateMine(struct EmHolder *h, s32 unused, f32 *pos);
u16 Rand_Next(s32 stream);
s32 EmChameleos_IsInvisible(Em *);
void ImpactRocks_Setup(StageObj *fx);
void ImpactRocks_DrawRock(DrawSlot *node);
void ImpactRocks_Run(StageObj *fx);
void ImpactRocks_End(StageObj *fx);
void ImpactRocks_Free(StageObj *fx);
void DemonGlow_Setup(StageObj *fx);
void DemonGlow_Run(StageObj *fx);
void DemonGlow_End(StageObj *fx);
void DemonGlow_Free(StageObj *fx);
void ThunderSpark_Setup(StageObj *fx);
void ThunderSpark_Run(StageObj *fx);
void ThunderSpark_End(StageObj *fx);
void ThunderSpark_Free(StageObj *fx);
void WeaponTrail_Setup(StageObj *fx);
void WeaponTrail_Draw(DrawSlot *node);
void WeaponTrail_Run(StageObj *fx);
void WeaponTrail_End(StageObj *fx);
void WeaponTrail_Free(StageObj *fx);
void EmBodyFx_Setup(StageObj *fx);
void EmBodyFx_Draw(DrawSlot *node);
u8 EmBodyFx_CoverMesh(Em *em);
void Vec_RotateY(f32 *v, f32 angle);
void EmBodyFx_Run(StageObj *fx);
void EmBodyFx_End(StageObj *fx);
void EmBodyFx_Free(StageObj *fx);
void EmTail_Setup(StageObj *fx);
void EmTail_SetDropPos(Em *em, StageObj *fx, s16 mode);
void EmTail_Run(StageObj *fx);
struct QuestEm *QuestEm_FindForEm(struct Em *em);
void MinePoint_SetPos(s32 slot, f32 *pos);
s32 MinePoint_Count(s32 slot);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void EmTail_Drop(StageObj *fx);
void EmTail_AddCarvePoint(StageObj *fx);
void EmTail_End(StageObj *fx);
void EmTail_Free(StageObj *fx);
void LaoshanDust_Setup(StageObj *fx);
void LaoshanDust_Run(StageObj *fx);
void LaoshanDust_End(StageObj *fx);
void LaoshanDust_Free(StageObj *fx);
void PlayerHitBurst_Setup(StageObj *fx);
void PlayerHitBurst_Run(StageObj *fx);
void PlayerHitBurst_End(StageObj *fx);
void PlayerHitBurst_Free(StageObj *fx);
void ItemProp_Setup(StageObj *o);
void ItemProp_Run(StageObj *o);
void ItemProp_SetupMeat(StageObj *o);
void ItemProp_RunMeat(StageObj *o);
void ItemProp_DrawMeat(StageObj *);
void ItemProp_SetupSmoke(StageObj *o);
void ItemProp_RunSmoke(StageObj *fx);
void SightBlock_Remove(EmObj *);
extern u16 ItemProp_SmokeRingSpins[];
void ItemProp_DrawSmoke(StageObj *fx, DrawSlot *node);
void ItemProp_SetupBait(StageObj *o);
void ItemProp_RunBait(StageObj *o);
void ItemProp_DrawBait(StageObj *fx);
void ItemProp_SetupDropped(StageObj *o);
void ItemProp_RunDropped(StageObj *fx);
void ItemProp_DrawDropped(StageObj *fx, DrawSlot *node);
void ItemProp_SetupPuffs(StageObj *o);
void ItemProp_RunPuffs(StageObj *o);
void ItemProp_DrawPuff(StageObj *o, DrawSlot *node);
void ItemProp_End(StageObj *o);
void ItemProp_Free(StageObj *o);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void StageObj_Free(struct StageObj *p);
void DrawSlot_Free(s32 i);
void DrawSlotB_Free(s32);
u16 Area_DayId(u16 id);
StageObj *WeaponTrail_Create(void *owner, s32 arg1, s32 kind);
f32 *Em_Bone(void *obj, s32 bone); /* unit's own declaration: declared here as the callers use it, returning the bone's matrix with an s32 bone; with the definition's EmBone *(void *obj, s16 bone), the calls compile differently */
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
extern s16 DemonGlow_BladeCounts[];
extern s16 ThunderSpark_PartCounts[];
extern s16 LaoshanDust_PieceCounts[];
extern s16 LaoshanDust_Lifetimes[];
extern s16 LaoshanDust_KeyStrides[];
extern s16 LaoshanDust_PieceEnds[];
extern s16 LaoshanDust_FirstKey[];
extern s16 *LaoshanDust_PieceEndLists[];
extern void *LaoshanDust_Keys[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
extern s16 WeaponTrail_Counts[];
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void MinePoint_Remove(s32);
void Vec_Normalize(f32 *);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *, f32 *, f32 *);
void Vec_Sub(f32 *, f32 *, f32 *);
extern f32 gCamPos;
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s16 id); /* unit's own declaration: declared with an s16 id (the definition takes an s32): with the definition's type, the set-ups that pass a draw slot number (WeaponTrail_Setup, EmTail_Setup, ItemProp_SetupBait) compile differently */
void ObjList_Remove(EmObj *obj);
void PlayerHitBurst_Draw(DrawSlot *node);
void Gfx_SetBlend(s32, s32);
s16 EmSlot_FindKind(s16 k);
s16 Player_DualSwordIndex(void *obj);
s32 SightBlock_Add(EmObj *obj);
void DemonGlow_DrawBlade(DrawSlot *node);
s32 Obj_InCurrentArea(void *obj);
void EmTail_Draw(DrawSlot *node);
s32 EmTail_OwnerInvisible(Em *em);
s32 EmTail_RootBone(Em *em);
void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
struct EmBone;
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
struct MtxSlot;
void Bone_StoreMatricesTo(struct MtxSlot *tbl, struct EmBone *b);
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Blend(f32 *dst, f32 *a, f32 *b, f32 wa, f32 wb);
f32 Vec_Length(f32 *v);
extern f32 EmBodyFx_ChargeBlink0[];
extern f32 EmBodyFx_ChargeBlink1[];
extern f32 EmBodyFx_FlashScaleKeys[];
extern u8 EmBodyFx_FlashColorKeys[];
void Key_LerpVec2(f32 *k, f32 *out, f32 t);
void Mtx_CopyRot(f32 *m, f32 *src);
void Mtx_RotateX(f32 *m, f32 a);
extern f32 WeaponTrail_RotWeights[][2];
extern f32 WeaponTrail_CurveWeights[][3];
extern s16 WeaponTrail_MeshIdx[];
void Tex_Prepare(s32 a, s32 b);
void GmEm_PrepareMeshSet(Em *em, s32 mode, ModelMatSet *sets);
void MtxSlot_LoadList(s16 *list, struct MtxSlot *tbl);
extern u8 gBoneMatrixBuf[];
extern u8 gModelPartLists[];
extern u8 EmTail_FlyOfsXKeys[];
extern u8 EmTail_FlyOfsYKeys[];
extern u8 EmTail_FlyOfsZKeys[];
extern u8 EmTail_FlyRotKeys[];
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s16 id); /* unit's own declaration: declared with an s16 id (the definition takes an s32): with the definition's type, the set-ups that pass a draw slot number (WeaponTrail_Setup, EmTail_Setup, ItemProp_SetupBait) compile differently */
void FireBlast_CreateOnEm(Em *em, s16 k);
u16 Mtx_Heading(f32 *m);
f32 AreaGrid2_GetFloorY(f32 *pos);
extern DemonGlowPair DemonGlow_OfsLeft[];
extern DemonGlowPair DemonGlow_OfsRight[];
extern DemonGlowPair DemonGlow_ScaleLeft[];
extern DemonGlowPair DemonGlow_ScaleLeftLen[];
extern DemonGlowPair DemonGlow_ScaleRight[];
extern DemonGlowPair DemonGlow_ScaleRightLen[];
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_TurnX(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
s32 Motion_ReachedFrame(void *, s32, f32);
extern ThunderSparkPoint ThunderSpark_BodyPoints[];
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
void Camera_SetFov(f32 fov);
void Camera_RestoreFov(void);
void Mtx_RotateZ(f32 *m, f32 a);
extern f32 ItemProp_SmokeRingScales[];
extern u8 ItemProp_SmokeColors[][4];
extern u8 ItemProp_SmokeColorsG[][4];
extern u8 ItemProp_SmokeColorsB[][4];
extern u8 ItemProp_BaitColorsFirst[][4];
extern u8 ItemProp_BaitColorsFirstG[][4];
extern u8 ItemProp_BaitColorsFirstB[][4];
extern u8 ItemProp_BaitColorsSecond[][4];
extern u8 ItemProp_BaitColorsSecondG[][4];
extern u8 ItemProp_BaitColorsSecondB[][4];
extern f32 EmBodyFx_PropOffsets[][3];
extern f32 EmBodyFx_RingOffsets[][3];
extern f32 WeaponTrail_DualScalesA[];
extern f32 WeaponTrail_DualScalesB[];
void WeaponTrail_GetBladeMatrix(f32 *m, StageObj *fx, Em *em, s16 alt);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
s32 Sound_PlaySeAtPosChecked(s32, s32, s32, void *, s32, s32);
void DemonGlow_BladeMatrix(f32 *m, StageObj *fx, Em *em, DemonGlowBlade *seg, s16 kind);
void DemonGlow_BladeOffset(f32 *out, StageObj *fx, Em *owner, DemonGlowBlade *seg);

extern Vec3f WeaponTrail_Offsets[];
extern Vec3f WeaponTrail_OffsetsY[];
extern Vec3f WeaponTrail_OffsetsZ[];
extern Vec3f WeaponTrail_DualOffsetsA[];
extern Vec3f WeaponTrail_DualOffsetsAY[];
extern Vec3f WeaponTrail_DualOffsetsAZ[];
extern Vec3f WeaponTrail_DualOffsetsB[];
extern Vec3f WeaponTrail_DualOffsetsBY[];
extern Vec3f WeaponTrail_DualOffsetsBZ[];
void WeaponTrail_GetBladeOffset(f32 *out, StageObj *fx, void *obj, s16 alt);
extern LaoshanDustBone LaoshanDust_Bones[];
void LaoshanDust_Draw(DrawSlot *node);
void ThunderSpark_BonePos(f32 *out, Em *em, f32 *ofs, s32 bone);
void ThunderSpark_DrawPart(DrawSlot *node);
extern s16 ThunderSpark_Lifetimes[];
extern s16 ThunderSpark_KeyStart[];
extern s16 ThunderSpark_KeyStep[];
extern s16 *ThunderSpark_PartEnds[];
extern void *ThunderSpark_Keys[];
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
f32 AreaGrid2_GetFloorYFiltered(f32 *pos);
void ThunderSpark_PickBodyPoints(StageObj *fx, ThunderSparkPart *p);
void ThunderSpark_RestartFanCharge(StageObj *fx, ThunderSparkPart *p);
void ThunderSpark_RestartKirinCharge(StageObj *fx, ThunderSparkPart *p);
void ThunderSpark_CreateAt(void *src, u8 k, u8 c, f32 f);
f32 Math_Sin(f32 x);
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmTail_SetDropPos compile differently */
void Em_BonePos(void *obj, s32 bone, f32 *out);
u8 EmKind38_IsCharging(Em *em);
extern u8 ThunderSpark_FanChargeColors0[];
extern u8 ThunderSpark_FanChargeColors1[];
extern u8 ThunderSpark_FanChargeColors2[];
extern u8 ThunderSpark_FanChargeColors3[];
extern u8 *ThunderSpark_FanBurstColors[];
extern u8 *ThunderSpark_GroundBoltColors[];
void Mtx_Identity(f32 *m);
void ThunderSpark_TurnToCamera(f32 *arg0, void *pos);
/* end of prototypes */

/*
 * Runs the state: STAGEOBJ_SETUP, RUN, END, FREE.
 */
void ImpactRocks_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        ImpactRocks_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        ImpactRocks_Run(obj);
        break;
    case STAGEOBJ_END:
        ImpactRocks_End(obj);
        break;
    case STAGEOBJ_FREE:
        ImpactRocks_Free(obj);
        break;
    }
}

/*
 * Starts the nine rocks at pos with random spins and an outward speed (type
 * 1 throws them 1.5 times higher), mirrored into the four quadrants by i %
 * 4, gravity making them peak at frame 15, a random mesh of five, and a draw
 * slot each (ImpactRocks_DrawRock, part = the rock).
 */
void ImpactRocks_Setup(StageObj *obj)
{
    ImpactRock *rock = (ImpactRock *) obj->buf;
    s32 i;

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    for (i = 0; i < 9; i++, rock++) {
        rock->pos[0] = obj->pos[0];
        rock->pos[1] = obj->pos[1];
        rock->pos[2] = obj->pos[2];
        rock->rot[0] = Rand_Next(1);
        rock->rot[1] = Rand_Next(1);
        rock->rot[2] = Rand_Next(1);
        switch (obj->type) {
        case IMPACT_ROCKS_LOW:
            rock->vel[0] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            rock->vel[1] = 2.0f * (5.0f + 0.4f * (Rand_Next(1) % 10));
            rock->vel[2] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            break;
        case IMPACT_ROCKS_HIGH:
            rock->vel[0] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            rock->vel[1] = 2.0f * (1.5f * (10.0f + 0.5f * (Rand_Next(1) % 10)));
            rock->vel[2] = 2.0f * (3.0f + 0.3f * (Rand_Next(1) % 10));
            break;
        }
        switch (i % 4) {
        case 0:
            break;
        case 1:
            rock->vel[0] *= -1.0f;
            rock->vel[2] *= -1.0f;
            break;
        case 2:
            rock->vel[0] *= -1.0f;
            break;
        case 3:
            rock->vel[2] *= -1.0f;
            break;
        }
        rock->acc[0] = 0.0f;
        rock->acc[2] = 0.0f;
        rock->acc[1] = 2.0f * (-1.0f * (rock->vel[1] / 15.0f));
        rock->mesh = Rand_Next(1) % 5;
        rock->slot = DrawSlot_Alloc();
        if (rock->slot != -1) {
            rock->node = DrawSlot_Get(rock->slot);
            rock->node->owner = obj;
            rock->node->part = i;
            rock->node->draw = ImpactRocks_DrawRock;
        } else {
            rock->node = NULL;
        }
    }
}

/*
 * Moves and spins each rock and puts its slot into gDrawOt32; after 30
 * frames hides the object and goes to the end state.
 */
void ImpactRocks_Run(StageObj *obj)
{
    ImpactRock *rock = (ImpactRock *) obj->buf;
    s32 i;

    if (obj->timer++ >= 30) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < 9; i++, rock++) {
        rock->pos[0] += rock->vel[0];
        rock->pos[1] += rock->vel[1];
        rock->pos[2] += rock->vel[2];
        rock->vel[0] += rock->acc[0];
        rock->vel[1] += rock->acc[1];
        rock->vel[2] += rock->acc[2];
        rock->rot[0] += 0x200;
        rock->rot[1] += 0x240;
        rock->rot[2] += 0x300;
        if (rock->node != NULL) {
            rock->node->pos[0] = rock->pos[0];
            rock->node->pos[1] = rock->pos[1];
            rock->node->pos[2] = rock->pos[2];
            OT_AddByDepth(gDrawOt32, rock->node, 0x20, 0);
        }
    }
}

/*
 * Frees the rocks' draw slots and goes to the free state.
 */
void ImpactRocks_End(StageObj *obj)
{
    ImpactRock *rock = (ImpactRock *) obj->buf;
    s32 i;

    obj->state++;
    for (i = 0; i < 9; i++) {
        if (rock->node != NULL) {
            DrawSlot_Free(rock->slot);
        }
        rock++;
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void ImpactRocks_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draw slot callback: one rock of gCommonModels[0] at its position and spin,
 * white, texture filter off; the mesh by var2: 0 one of meshes 32-36 by the
 * rock's mesh index, 1 mesh 43 (FallingDebris type 0's), else mesh 73
 * (AttackBurst's rock).
 */
void ImpactRocks_DrawRock(DrawSlot *node)
{
    StageObj *obj = node->owner;
    ImpactRock *rock = (ImpactRock *) obj->buf + node->part;
    ModelMat *materials;
    Model *set = gCommonModels[0];
    ModelMatSet *mesh;
    f32 mtx[16];

    if (set != NULL && set->active != 0) {
        materials = set->mats;
        Gfx_SetTexFilter(0);
        Mtx_Translation(mtx, rock->pos[0], rock->pos[1], rock->pos[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * rock->rot[0]) / 65536.0f, (6.2831855f * rock->rot[1]) / 65536.0f, (6.2831855f * rock->rot[2]) / 65536.0f);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, -1);
        if (obj->var2 == IMPACT_ROCKS_MESH_RANDOM) {
            mesh = &set->matSets[rock->mesh] + 32;
        } else if (obj->var2 == IMPACT_ROCKS_MESH_DEBRIS) {
            mesh = &set->matSets[43];
        } else {
            mesh = &set->matSets[73];
        }
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(materials, mesh);
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags(0x10);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
        Gfx_SetTexFilter(1);
    }
}

/*
 * Creates the rocks at pos when the owner (a player) is in the current area:
 * type is the throw height, var2 the mesh choice.
 */
void ImpactRocks_Create(void *owner, u8 type, f32 *pos, u8 meshChoice)
{
    StageObj *obj;

    if (Player_InCurrentArea(owner) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_IMPACT_ROCKS;
            obj->update = ImpactRocks_Update;
            obj->pos[0] = pos[0];
            obj->pos[1] = pos[1];
            obj->pos[2] = pos[2];
            obj->owner = owner;
            obj->type = type;
            obj->var2 = meshChoice;
        }
    }
}

/*
 * Runs the state: STAGEOBJ_SETUP, RUN, END, FREE.
 */
void DemonGlow_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        DemonGlow_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        DemonGlow_Run(obj);
        break;
    case STAGEOBJ_END:
        DemonGlow_End(obj);
        break;
    case STAGEOBJ_FREE:
        DemonGlow_Free(obj);
        break;
    }
}

/*
 * Stores the owner's dual sword index (Player_DualSwordIndex) and gives each
 * blade (DemonGlow_BladeCounts[type], two) a slot of the second draw pool
 * (DrawSlotB, DemonGlow_DrawBlade); frees the object at once when the owner
 * is inactive or holds no dual swords.
 */
void DemonGlow_Setup(StageObj *obj)
{
    DemonGlowBlade *blade;
    Em *pl;
    s16 count;
    s16 i;

    blade = (DemonGlowBlade *) obj->buf;
    pl = obj->owner;
    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    count = DemonGlow_BladeCounts[obj->type];
    if (pl->active == 0 || (((DemonGlowBuf *) blade)->sword = Player_DualSwordIndex(pl)) == -1) {
        obj->visible = 0;
        DemonGlow_Free(obj);
        return;
    }
    for (i = 0; i < count; i++, blade++) {
        blade->side = i;
        blade->blink = i & 1;
        blade->slot = DrawSlotB_Alloc();
        if (blade->slot != -1) {
            blade->node = DrawSlotB_Get(blade->slot);
            blade->node->owner = obj;
            blade->node->part = i;
            blade->node->draw = DemonGlow_DrawBlade;
        } else {
            blade->node = NULL;
        }
    }
}

/*
 * Each frame toggles each blade's blink and puts the blade at its hand bone
 * plus the side offset, drawn (gDrawOtSorted) on alternate frames. Draws
 * nothing while the player is hidden or in another area, in PL_GROUP_REACT
 * or PL_GROUP_GESTURE, out of demon mode (PL_STATUS_ICON_DEMON) except in
 * PL_MOT_DSWORDS_DEMON_LEAVE before frame 22, in PL_MOT_DSWORDS_DEMON_ENTER
 * before frame 32, or offering an item (PL_MOT_OFFER_ITEM, _END); ends when
 * the player is inactive.
 */
void DemonGlow_Run(StageObj *obj)
{
    DemonGlowBlade *blade = (DemonGlowBlade *) obj->buf;
    Em *pl = obj->owner;
    s16 count;
    s16 i;
    f32 mtx[16];
    f32 ofs[3];

    if (pl->active == 0) {
        obj->visible = 0;
        obj->state++;
        return;
    }
    if (pl->visible == 0 || !(Obj_InCurrentArea(pl) & 0xFF)) {
        return;
    }
    if (pl->stateGroup == PL_GROUP_REACT || pl->stateGroup == PL_GROUP_GESTURE) {
        return;
    }
    if ((!(pl->statusIcons & PL_STATUS_ICON_DEMON) && (pl->motionId[0] != PL_MOT_DSWORDS_DEMON_LEAVE || Motion_ReachedFrame(pl, 0, 22.0f))) ||
        (pl->motionId[0] == PL_MOT_DSWORDS_DEMON_ENTER && !Motion_ReachedFrame(pl, 0, 32.0f)) || pl->motionId[0] == PL_MOT_OFFER_ITEM ||
        pl->motionId[0] == PL_MOT_OFFER_ITEM_END) {
        return;
    }
    count = DemonGlow_BladeCounts[obj->type];
    for (i = 0; i < count; i++, blade++) {
        blade->blink ^= 1;
        DemonGlow_BladeOffset(ofs, obj, pl, blade);
        DemonGlow_BladeMatrix(mtx, obj, pl, blade, 0);
        Vec_RotateSelf(ofs, mtx);
        blade->pos[0] = mtx[12] + ofs[0];
        blade->pos[1] = mtx[13] + ofs[1];
        blade->pos[2] = mtx[14] + ofs[2];
        if ((blade->blink & 1) && blade->node != NULL) {
            blade->node->pos[0] = blade->pos[0];
            blade->node->pos[1] = blade->pos[1];
            blade->node->pos[2] = blade->pos[2];
            OT_AddByDepth(gDrawOtSorted, blade->node, 0x40, 0);
        }
    }
}

/*
 * Frees the blades' draw slots (only the first blade's, see NOTE) and goes
 * to the free state.
 */
void DemonGlow_End(StageObj *obj)
{
    DemonGlowBlade *blade = (DemonGlowBlade *) obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = DemonGlow_BladeCounts[obj->type];
    for (i = 0; i < count; i++) { /* blade is never stepped: the first blade's slot is freed count times, the second's never */
        if (blade->node != NULL) {
            DrawSlotB_Free(blade->slot);
        }
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void DemonGlow_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draw slot callback: mesh 59 of gCommonModels[0] with the blade's scaled
 * bone matrix (DemonGlow_BladeMatrix 1) at the slot's position, tint
 * 0x33FF3333 (faint red), blend flags 2, Gs state 0x6C set to 0 before and
 * after; nothing while the player is hidden.
 */
void DemonGlow_DrawBlade(DrawSlot *node)
{
    StageObj *obj = node->owner;
    DemonGlowBlade *blade = (DemonGlowBlade *) obj->buf + node->part;
    ModelMat *materials;
    Model *set = gCommonModels[0];
    Em *pl = obj->owner;
    ModelMatSet *mesh;
    f32 mtx[16];

    if (set == NULL || pl->visible == 0) {
        return;
    }
    if (set->active != 0) {
        materials = set->mats;
        DemonGlow_BladeMatrix(mtx, obj, pl, blade, 1);
        Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        mesh = &set->matSets[59];
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x67, 0x33FF3333);
        if (mesh != NULL && mesh->id != -1) {
            Gs_SetLightList(materials, mesh);
            Draw_ApplyStates(mesh->flags);
            Draw_SetBlendFlags(2);
            Model_Draw(mesh->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 0);
    }
}

/*
 * The blade's matrix: the left hand bone 0xE (blade 0) or the right 0x12
 * (blade 1, turned half a turn about z); with scaled = 1 also turned a
 * quarter turn about x and its x and y axes scaled by the sword's row of
 * DemonGlow_ScaleLeft and DemonGlow_ScaleLeftLen (DemonGlow_ScaleRight and
 * DemonGlow_ScaleRightLen for the right blade).
 */
void DemonGlow_BladeMatrix(f32 *mtx, StageObj *obj, Em *pl, DemonGlowBlade *blade, s16 scaled)
{
    DemonGlowBuf *buf = (DemonGlowBuf *) obj->buf;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;

    switch (scaled) {
    case 0:
        if (blade->side == 0) {
            Mtx_Copy(mtx, (f32 *) Em_GetBone(pl, 0xE));
        } else {
            Mtx_Copy(mtx, (f32 *) Em_GetBone(pl, 0x12));
        }
        break;
    case 1:
        if (blade->side == 0) {
            scaleX = DemonGlow_ScaleLeft[buf->sword].x;
            scaleY = DemonGlow_ScaleLeftLen[buf->sword].x;
            scaleZ = 1.0f;
            Mtx_Copy(mtx, (f32 *) Em_GetBone(pl, 0xE));
        } else {
            scaleX = DemonGlow_ScaleRight[buf->sword].x;
            scaleY = DemonGlow_ScaleRightLen[buf->sword].x;
            scaleZ = 1.0f;
            Mtx_Copy(mtx, (f32 *) Em_GetBone(pl, 0x12));
            Mtx_TurnZ(mtx, 3.1415927f);
        }
        Mtx_TurnX(mtx, 1.5707964f);
        mtx[0] *= scaleX;
        mtx[1] *= scaleX;
        mtx[2] *= scaleX;
        mtx[4] *= scaleY;
        mtx[5] *= scaleY;
        mtx[6] *= scaleY;
        mtx[8] *= scaleZ;
        mtx[9] *= scaleZ;
        mtx[10] *= scaleZ;
        break;
    }
}

/*
 * The glow's offset from the hand bone, by the sword's row: left x = 10 +
 * OffsetL.x, right x = -OffsetR.x - 10, z from the table, y 0.
 */
void DemonGlow_BladeOffset(f32 *out, StageObj *obj, Em *pl, DemonGlowBlade *blade)
{
    DemonGlowBuf *buf = (DemonGlowBuf *) obj->buf;

    if (blade->side == 0) {
        out[0] = 10.0f + DemonGlow_OfsLeft[buf->sword].x;
        out[1] = 0.0f;
        out[2] = DemonGlow_OfsLeft[buf->sword].z;
    } else {
        out[0] = -DemonGlow_OfsRight[buf->sword].x - 10.0f;
        out[1] = 0.0f;
        out[2] = DemonGlow_OfsRight[buf->sword].z;
    }
}

/*
 * Creates the glow for a player (kind 3, type k, owner and ownerIndex the
 * player's, keep = 1).
 */
void DemonGlow_Create(Em *pl, u8 type)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_DEMON_GLOW;
        obj->update = DemonGlow_Update;
        obj->draw = NULL;
        obj->type = type;
        obj->owner = pl;
        obj->ownerIndex = pl->index;
        obj->keep = 1;
    }
}

/*
 * Runs the state: STAGEOBJ_SETUP, RUN, END, FREE.
 */
void ThunderSpark_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        ThunderSpark_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        ThunderSpark_Run(obj);
        break;
    case STAGEOBJ_END:
        ThunderSpark_End(obj);
        break;
    case STAGEOBJ_FREE:
        ThunderSpark_Free(obj);
        break;
    }
}

/*
 * Sets up ThunderSpark_PartCounts[type] parts by type (look, spin, size, start
 * delay, draw list; types 4 and 9 first put pos at the head, bone 0x22) with
 * a draw slot each (ThunderSpark_DrawPart), then runs the first
 * ThunderSpark_Run.
 */
void ThunderSpark_Setup(StageObj *obj)
{
    ThunderSparkPart *part;
    Em *em;
    s16 count;
    f32 ofs[3];
    f32 sizeScale;
    s16 i;

    part = (ThunderSparkPart *) obj->buf;
    em = obj->owner;
    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    count = ThunderSpark_PartCounts[obj->type];
    switch (obj->type) {
    case SPARK_KHEZU_FAN_BURST:
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 100.0f;
        ThunderSpark_BonePos(obj->pos, em, ofs, 0x22);
        break;
    case SPARK_RED_KHEZU_DROP:
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 80.0f;
        ThunderSpark_BonePos(obj->pos, em, ofs, 0x22);
        break;
    }
    for (i = 0; i < count; part++, i++) {
        part->frame = 0;
        part->look = i;
        part->keyScale[0] = 1.0f;
        part->keyScale[1] = 1.0f;
        part->keyScale[2] = 1.0f;
        part->scale = obj->scale;
        switch (obj->type) {
        case SPARK_KHEZU_DISCHARGE:
            sizeScale = em->sizePct / 100.0f;
            if (i == 0) {
                part->look = 0;
                part->rot[0] = Rand_Next(1);
                part->rot[1] = Rand_Next(1);
                part->rot[2] = Rand_Next(1);
                part->scale = 6.0f * sizeScale;
                part->sorted = SPARK_DRAW_ADD;
            } else {
                part->look = 1;
                part->rot[2] = Rand_Next(1);
                part->scale = 2.0f * sizeScale;
                part->sorted = SPARK_DRAW_SORTED;
            }
            break;
        case SPARK_KHEZU_SHOT_FLASH:
            part->rot[2] = Rand_Next(1);
            part->sorted = SPARK_DRAW_SORTED;
            Vec_Copy(part->pos, obj->pos);
            break;
        case SPARK_KHEZU_SHOT_RISE:
            part->frame = -2 - i * 2;
            part->rot[2] = Rand_Next(1);
            part->sorted = SPARK_DRAW_SORTED;
            part->pos[0] = obj->pos[0] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            part->pos[1] = obj->pos[1] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            part->pos[2] = obj->pos[2] + 0.010000001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            Vec_Copy(part->pos, obj->pos);
            part->scale *= 1.2f - 0.2f * i;
            part->rise = (Rand_Next(1) & 0x1F) + 0x4B;
            break;
        case SPARK_KHEZU_FAN_CHARGE:
            if (i == 4) {
                part->look = 3;
                part->frame = -5;
            }
            part->rot[2] = Rand_Next(1);
            if (part->look == 2) {
                part->sorted = SPARK_DRAW_ADD;
            } else {
                part->sorted = SPARK_DRAW_SORTED;
            }
            break;
        case SPARK_KHEZU_FAN_BURST:
            switch (i) {
            case 0:
                part->rot[0] = 0;
                part->rot[1] = em->rotY;
                break;
            case 1:
                part->rot[0] = 0;
                part->rot[1] = em->rotY;
                part->scale *= 0.75f;
                break;
            case 2:
                part->rot[2] = Rand_Next(1);
                break;
            case 4:
                part->frame = -1;
                part->rot[2] = Rand_Next(1);
                break;
            case 5:
                part->frame = -4;
                part->rot[1] = em->rotY + 0x8000;
                part->scale *= 0.5f;
                break;
            }
            part->sorted = SPARK_DRAW_SORTED;
            Vec_Copy(part->pos, obj->pos);
            part->scale *= 2.0f;
            break;
        case SPARK_KHEZU_GROUND_BOLT:
            if (i == 1) {
                part->look = 0;
                part->frame = -2;
            } else if (i == 2) {
                part->look = 1;
            }
            part->rot[2] = Rand_Next(1);
            part->sorted = SPARK_DRAW_SORTED;
            Vec_Copy(part->pos, obj->pos);
            break;
        case SPARK_KIRIN_BOLT_SKY:
            part->frame = -i * 3;
            part->rot[2] = Rand_Next(1);
            part->sorted = SPARK_DRAW_SORTED;
            Vec_Copy(part->pos, obj->pos);
            break;
        case SPARK_KIRIN_BOLT_GROUND:
            part->frame = -4;
            Vec_Copy(part->pos, obj->pos);
            switch (part->look) {
            case 0:
                part->rot[2] = Rand_Next(1);
                break;
            case 2:
                part->rot[1] = Rand_Next(1);
                part->pos[1] += 5.0f;
                break;
            }
            part->sorted = SPARK_DRAW_SORTED;
            break;
        case SPARK_KIRIN_CHARGE:
            switch (part->look) {
            case 0:
                part->rot[0] = Rand_Next(1);
                part->rot[1] = Rand_Next(1);
                part->rot[2] = Rand_Next(1);
                part->scale *= 0.5f;
                part->sorted = SPARK_DRAW_ADD;
                break;
            default:
                part->rot[2] = Rand_Next(1);
                part->scale *= 3.0f;
                part->sorted = SPARK_DRAW_SORTED;
                break;
            }
            break;
        case SPARK_RED_KHEZU_DROP:
            part->keyScale[1] *= 0.33f;
            part->vel[0] = 0;
            part->vel[1] = 0;
            part->vel[2] = 0;
            Vec_Copy(part->pos, obj->pos);
            part->pos[0] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            part->pos[1] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            part->pos[2] += 30.0f * (0.001f * (Rand_Next(1) & 0x3FF) - 0.5f);
            part->sorted = SPARK_DRAW_SORTED;
            break;
        }
        part->slot = DrawSlot_Alloc();
        if (part->slot != -1) {
            part->node = DrawSlot_Get(part->slot);
            part->node->owner = obj;
            part->node->part = i;
            part->node->draw = ThunderSpark_DrawPart;
        } else {
            part->node = NULL;
        }
    }
    ThunderSpark_Run(obj);
}

/*
 * Counts timer to the type's lifetime (types 3 and 8: angle), ending early
 * when the owner or its motion changes (types 0, 3, 8) or all drops landed
 * (9). Per part counts its frame, keys its scale and alpha from the type's
 * curves while the frame is 1..its end, moves it by type and puts its slot
 * into gDrawOtSorted (sorted parts) or gDrawOt32.
 */
void ThunderSpark_Run(StageObj *obj)
{
    s16 i;
    ThunderSparkPart *part;
    Em *em;
    s16 count;
    s16 life;
    s16 *partEnds;
    s16 partEnd;
    s16 key;
    s16 keyStep;
    s32 firstKey;
    u8 dim;
    f32 ofs[3];
    f32 alphaKey;
    f32 floorY;
    void *keys;

    part = (ThunderSparkPart *) obj->buf;
    em = obj->owner;
    count = ThunderSpark_PartCounts[obj->type];
    life = ThunderSpark_Lifetimes[obj->type];
    partEnds = ThunderSpark_PartEnds[obj->type];
    i = ThunderSpark_KeyStep[obj->type];
    key = ThunderSpark_KeyStart[obj->type];
    keyStep = i;
    if (obj->type == SPARK_KHEZU_FAN_CHARGE || obj->type == SPARK_KIRIN_CHARGE) {
        life = (u16) obj->angle;
    }
    if (++obj->timer > life) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    switch (obj->type) {
    case SPARK_KHEZU_DISCHARGE:
        if (em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x45A) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        ThunderSpark_PickBodyPoints(obj, part);
        break;
    case SPARK_KHEZU_SHOT_RISE:
        partEnd = 0x16;
        break;
    case SPARK_KHEZU_FAN_CHARGE:
        if (em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x417) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 100.0f;
        ThunderSpark_BonePos(obj->pos, em, ofs, 0x22);
        break;
    case SPARK_KIRIN_BOLT_GROUND:
        if (obj->timer == life) {
            PartFx_SpawnAt(obj->pos, 0x1B, 1.0f);
        }
        break;
    case SPARK_KIRIN_CHARGE:
        if (em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x40F) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        ofs[0] = 0.0f;
        ofs[1] = 14.0f;
        ofs[2] = 24.0f;
        ThunderSpark_BonePos(obj->pos, em, ofs, 0x17);
        break;
    case SPARK_RED_KHEZU_DROP:
        for (i = 0; i < count; i++) {
            if (part[i].look != SPARK_LOOK_LANDED) {
                break;
            }
        }
        if (i >= count) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        break;
    }
    for (i = 0; i < count; i++, part++) {
        part->frame++;
        if (partEnds != NULL) {
            partEnd = partEnds[i];
        } else {
            key = ThunderSpark_KeyStart[obj->type];
        }
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 0.0f;
        switch (obj->type) {
        case SPARK_KHEZU_DISCHARGE:
            if (part->look == 0) {
                if (!EmMotion_ReachedFrame(em, 0, 115.0f)) {
                    dim = 1;
                    continue;
                }
                dim = 0;
                if (part->frame >= 4) {
                    part->frame = 0;
                    part->rot[0] = Rand_Next(1);
                    part->rot[1] = Rand_Next(1);
                    part->rot[2] = Rand_Next(1);
                }
                ThunderSpark_BonePos(part->pos, em, ofs, 3);
            } else if (dim) {
                part->sorted = SPARK_DRAW_SORTED_DIM;
            } else {
                part->sorted = SPARK_DRAW_SORTED;
            }
            break;
        case SPARK_KHEZU_SHOT_FLASH:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            firstKey = key;
            Key_LerpVec3(ThunderSpark_Keys[firstKey], part->keyScale, part->frame);
            key += 2;
            Key_LerpFloat(ThunderSpark_Keys[(s16) (firstKey + 1)], &alphaKey, part->frame);
            part->alpha = 255.0f * alphaKey;
            if (part->look == 0) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = 20.0f;
                Vec_RotateSelf(ofs, gCamMatrix);
            } else {
                part->pos[1] -= 50.0f / partEnd;
            }
            break;
        case SPARK_KHEZU_SHOT_RISE:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            firstKey = key;
            Key_LerpVec3(ThunderSpark_Keys[firstKey], part->keyScale, part->frame);
            key += 2;
            Key_LerpFloat(ThunderSpark_Keys[(s16) (firstKey + 1)], &alphaKey, part->frame);
            part->alpha = 255.0f * alphaKey;
            part->pos[1] += obj->scale * (part->rise / (f32) partEnd);
            break;
        case SPARK_KHEZU_FAN_CHARGE:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                ThunderSpark_RestartFanCharge(obj, part);
                if (part->frame <= 0) {
                    key += keyStep;
                    continue;
                }
            }
            keys = ThunderSpark_Keys[key++];
            Key_LerpVec3(keys, part->keyScale, part->frame);
            Vec_Copy(part->pos, obj->pos);
            break;
        case SPARK_KHEZU_FAN_BURST:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            keys = ThunderSpark_Keys[key++];
            Key_LerpVec3(keys, part->keyScale, part->frame);
            break;
        case SPARK_KHEZU_GROUND_BOLT:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            keys = ThunderSpark_Keys[key++];
            Key_LerpVec3(keys, part->keyScale, part->frame);
            break;
        case SPARK_KIRIN_BOLT_SKY:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            keys = ThunderSpark_Keys[key++];
            Key_LerpVec3(keys, part->keyScale, part->frame);
            switch (part->look) {
            case 0:
                part->rot[2] = Rand_Next(1);
                break;
            case 1:
                part->rot[2] += 0x1000;
                break;
            }
            break;
        case SPARK_KIRIN_BOLT_GROUND:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                key += keyStep;
                continue;
            }
            firstKey = key;
            Key_LerpVec3(ThunderSpark_Keys[firstKey], part->keyScale, part->frame);
            key += 2;
            Key_LerpFloat(ThunderSpark_Keys[(s16) (firstKey + 1)], &alphaKey, part->frame);
            part->alpha = 255.0f * alphaKey;
            switch (part->look) {
            case 0:
                part->rot[2] += 0x1000;
                break;
            }
            break;
        case SPARK_KIRIN_CHARGE:
            if (part->frame <= 0) {
                key += keyStep;
                continue;
            }
            if (partEnd < part->frame) {
                ThunderSpark_RestartKirinCharge(obj, part);
            }
            Vec_Copy(part->pos, obj->pos);
            break;
        case SPARK_RED_KHEZU_DROP:
            if (part->look == SPARK_LOOK_LANDED) {
                continue;
            }
            part->vel[1] += -8.0f;
            part->pos[0] += part->vel[0];
            part->pos[1] += part->vel[1];
            part->pos[2] += part->vel[2];
            floorY = AreaGrid2_GetFloorYFiltered(part->pos);
            if (part->pos[1] <= floorY) {
                part->pos[1] = floorY;
                if (em->active != 0 && em->type == obj->var2) {
                    Em_PlaySeChecked(em, 0x1E, 0, part->pos, 1, 0);
                }
                ThunderSpark_CreateAt(part->pos, SPARK_KHEZU_SHOT_FLASH, obj->var2, 0.3f * obj->scale);
                ThunderSpark_CreateAt(part->pos, SPARK_KHEZU_SHOT_RISE, obj->var2, 0.3f * obj->scale);
                part->look = SPARK_LOOK_LANDED;
                continue;
            }
            break;
        }
        if (part->node != NULL) {
            part->node->pos[0] = part->pos[0] + ofs[0];
            part->node->pos[1] = part->pos[1] + ofs[1];
            part->node->pos[2] = part->pos[2] + ofs[2];
            if (part->sorted != SPARK_DRAW_ADD) {
                OT_AddByDepth(gDrawOtSorted, part->node, 0x40, 0);
            } else {
                OT_AddByDepth(gDrawOt32, part->node, 0x20, 0);
            }
        }
    }
}

/*
 * Frees the object's own slot if any (never allocated: node is NULL) and the
 * parts' draw slots, and goes to the free state.
 */
void ThunderSpark_End(StageObj *obj)
{
    ThunderSparkPart *part = (ThunderSparkPart *) obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = ThunderSpark_PartCounts[obj->type];
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
    for (i = 0; i < count; i++) {
        if (part->node != NULL) {
            DrawSlot_Free(part->slot);
        }
        part++;
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void ThunderSpark_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
/*
 * Draw slot callback: one part as a mesh of the kind's part models (nothing
 * when they are not loaded), by type and look: matrix from scale times keyed
 * scale, spin and position, facing the camera (gCamMatrix) or turned by the
 * camera's yaw (gCamYawMtx) by mode; colour white, alpha-keyed or from
 * colour keys (type 3 fades in over its first 20 frames); sorted parts drawn
 * with Effect_DrawModel, the others with Effect_DrawModelAdd.
 */
void ThunderSpark_DrawPart(DrawSlot *node)
{
    StageObj *obj;
    ThunderSparkPart *part;
    s16 kindSlot;
    Model *models;
    ModelMat *materials;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 texMtx[16];
    u32 color;
    ModelMatSet *mesh;
    u16 flags;
    s32 rotOrder;
    u8 facing;
    u8 *colorKeys;
    s32 rotOrder2;
    u8 alpha;

    flags = 0;
    obj = node->owner;
    part = (ThunderSparkPart *) obj->buf + node->part;
    kindSlot = EmSlot_FindKind(obj->var2);
    if (kindSlot != -1 && (models = gScene.partModels[kindSlot]) != NULL && models->active != 0) {
        materials = models->mats;
        scale[0] = part->scale * part->keyScale[0];
        scale[1] = part->scale * part->keyScale[1];
        scale[2] = part->scale * part->keyScale[2];
        facing = 1;
        switch (obj->type) {
        case SPARK_KHEZU_DISCHARGE:
            if (part->look == 0) {
                mesh = &models->matSets[7];
                rot[0] = (6.2831855f * part->rot[0]) / 65536.0f;
                rot[1] = (6.2831855f * part->rot[1]) / 65536.0f;
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 6;
                Mtx_Translation(texMtx, 0.0f, 0.046875f * part->frame, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                alpha = 0xFF;
                facing = 0;
            } else {
                mesh = models->matSets;
                flags |= 2;
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 3;
                alpha = 0xFF;
                if (part->sorted == SPARK_DRAW_SORTED) {
                    alpha = 0xFF;
                } else {
                    alpha = 0x80;
                    scale[0] *= 0.8f;
                    scale[1] *= 0.8f;
                    scale[2] *= 0.8f;
                }
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder, mtx);
            color = (alpha << 24) | 0xFFFFFF;
            goto draw;
        case SPARK_KHEZU_SHOT_FLASH:
            if (part->look == 0) {
                mesh = &models->matSets[3];
                flags |= 2;
            } else {
                mesh = &models->matSets[11];
            }
            rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            color = (part->alpha << 24) | 0xFFFFFF;
            goto draw;
        case SPARK_KHEZU_SHOT_RISE:
            mesh = &models->matSets[10];
            rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            color = (part->alpha << 24) | 0xFFFFFF;
            goto draw;
        case SPARK_KHEZU_FAN_CHARGE:
            switch (part->look) {
            case 0:
                colorKeys = ThunderSpark_FanChargeColors0;
                mesh = &models->matSets[2];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 3;
                flags |= 2;
                break;
            case 1:
                colorKeys = ThunderSpark_FanChargeColors1;
                rotOrder = 0;
                mesh = &models->matSets[3];
                flags |= 2;
                break;
            case 2:
                colorKeys = ThunderSpark_FanChargeColors2;
                mesh = &models->matSets[7];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 3;
                Mtx_Translation(texMtx, 0.0f, 0.0234375f * part->frame, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                facing = 0;
                break;
            case 3:
                colorKeys = ThunderSpark_FanChargeColors3;
                mesh = &models->matSets[5];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 3;
                flags |= 2;
                break;
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder, mtx);
            Color_EvalKeys((s32 *)colorKeys, part->frame, &color);
            if (obj->timer < 20) {
                /* fadeAlpha is set twice, so it stays a value of its own: its mask is made before the
                   red channel and its shift after it (the game's registers at the join) */
                u32 fadeAlpha = (color >> 24) & 0xFF;

                fadeAlpha = (u32) (fadeAlpha * (obj->timer / 20.0f)) & 0xFF;
                color = (color & 0xFF) | ((((color >> 8) & 0xFF) << 8) | ((fadeAlpha << 24) | (((color >> 16) & 0xFF) << 16)));
            }
            goto draw;
        case SPARK_KHEZU_FAN_BURST:
            switch (part->look) {
            case 0:
            case 1:
                mesh = &models->matSets[1];
                Mtx_Identity(mtx);
                Mtx_RotateXYZ(mtx, (6.2831855f * part->rot[0]) / 65536.0f, (6.2831855f * part->rot[1]) / 65536.0f, 0.0f);
                ThunderSpark_TurnToCamera(mtx, node->pos);
                mtx[0] *= part->keyScale[0];
                mtx[1] *= part->keyScale[0];
                mtx[2] *= part->keyScale[0];
                mtx[4] *= part->keyScale[1];
                mtx[5] *= part->keyScale[1];
                mtx[6] *= part->keyScale[1];
                mtx[8] *= part->keyScale[2];
                mtx[9] *= part->keyScale[2];
                mtx[10] *= part->keyScale[2];
                Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
                flags |= 2;
                facing = 0;
                break;
            case 2:
                mesh = &models->matSets[2];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                break;
            case 3:
                mesh = &models->matSets[3];
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
                break;
            case 4:
                mesh = &models->matSets[4];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                flags |= 2;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                break;
            case 5:
                mesh = &models->matSets[8];
                rot[1] = (6.2831855f * part->rot[1]) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, mtx);
                Mtx_Translation(texMtx, 0.0f, 0.0234375f * part->frame, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                facing = 0;
                break;
            }
            colorKeys = ThunderSpark_FanBurstColors[part->look];
            if (colorKeys == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *)colorKeys, part->frame, &color);
            }
            goto draw;
        case SPARK_KHEZU_GROUND_BOLT:
            switch (part->look) {
            case 0:
                mesh = &models->matSets[5];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder = 3;
                flags |= 2;
                break;
            case 1:
                mesh = &models->matSets[3];
                flags |= 2;
                rotOrder = 0;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
                break;
            }
            colorKeys = ThunderSpark_GroundBoltColors[part->look];
            if (colorKeys == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *)colorKeys, part->frame, &color);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder, mtx);
            goto draw;
        case SPARK_KIRIN_BOLT_SKY:
            mesh = &models->matSets[6];
            rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
            flags |= 2;
            color = 0xFFFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Translation(texMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            goto draw;
        case SPARK_KIRIN_BOLT_GROUND:
            Gs_SetState(0x6C, 0);
            switch (part->look) {
            case 0:
                mesh = &models->matSets[6];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder2 = 3;
                Mtx_Translation(texMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                break;
            case 1:
                rotOrder2 = 0;
                mesh = &models->matSets[4];
                break;
            case 2:
                mesh = &models->matSets[7];
                rot[1] = (6.2831855f * part->rot[1]) / 65536.0f;
                rotOrder2 = 2;
                facing = 0;
                break;
            default:
                return;
            }
            flags |= 2;
            color = (part->alpha << 24) | 0xFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder2, mtx);
            goto draw;
        case SPARK_KIRIN_CHARGE:
            Gs_SetState(0x6C, 0);
            switch (part->look) {
            case 0:
                mesh = models->matSets;
                rot[0] = (6.2831855f * part->rot[0]) / 65536.0f;
                rot[1] = (6.2831855f * part->rot[1]) / 65536.0f;
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder2 = 6;
                facing = 0;
                Mtx_Translation(texMtx, 0.0f, (0.1875f * part->frame) / 10.0f, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                break;
            default:
                flags |= 2;
                mesh = &models->matSets[6];
                rot[2] = (6.2831855f * part->rot[2]) / 65536.0f;
                rotOrder2 = 3;
                Mtx_Translation(texMtx, 0.0f, 0.25f, 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                break;
            }
            color = 0xFFFFFFFF;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder2, mtx);
            goto draw;
        case SPARK_RED_KHEZU_DROP:
            mesh = &models->matSets[9];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
            color = 0xFFFFFFFF;
            facing = 2;
            goto draw;
        default:
        draw:
            switch (facing) {
            case 1:
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            case 2:
                Mtx_Concat3x3(mtx, gCamYawMtx);
                break;
            }
            if (part->sorted != SPARK_DRAW_ADD) {
                Effect_DrawModel(mesh, mtx, color, flags, materials);
            } else {
                Gs_SetState(0x67, color);
                Effect_DrawModelAdd(mesh, mtx, materials);
            }
            Gs_SetState(0x6C, 1);
            break;
        }
    }
}

/*
 * Writes to out the point ofs in the monster's bone frame (Em_Bone).
 */
void ThunderSpark_BonePos(f32 *out, Em *em, f32 *ofs, s32 bone)
{
    f32 rotOfs[3];
    f32 mtx[16];

    Mtx_Copy(mtx, Em_Bone(em, bone));
    Vec_Rotate(rotOfs, ofs, mtx);
    out[0] = mtx[12] + rotOfs[0];
    out[1] = mtx[13] + rotOfs[1];
    out[2] = mtx[14] + rotOfs[2];
}

/*
 * Type 0: gives parts 1-4 four different points of the ten in
 * ThunderSpark_BodyPoints (each pick shifted past the ones taken), a new spin, the
 * point's size +-10 percent and its position; run every frame, so the sparks
 * jump over the body.
 */
void ThunderSpark_PickBodyPoints(StageObj *obj, ThunderSparkPart *part)
{
    s16 point[4];
    s16 taken[4];
    s16 i;
    s16 j;
    s16 shift;

    part++;
    for (i = 0; i < 4; i++, part++) {
        point[i] = Rand_Next(1) % (10 - i);
        for (j = 0; j < i; j++) {
            taken[j] = 0;
        }
        for (;;) {
            shift = 0;
            for (j = 0; j < i; j++) {
                if (taken[j] == 0 && point[j] <= point[i]) {
                    taken[j] = 1;
                    shift++;
                }
            }
            if (shift <= 0) {
                break;
            }
            point[i] += shift;
        }
        part->point = point[i];
        part->rot[2] = Rand_Next(1);
        part->scale = ThunderSpark_BodyPoints[part->point].scale * (0.9f + 2.0000001e-4f * (Rand_Next(1) & 0x3FF));
        ThunderSpark_BonePos(part->pos, obj->owner, ThunderSpark_BodyPoints[part->point].ofs, ThunderSpark_BodyPoints[part->point].bone);
    }
}

/*
 * Type 3: restarts a part past its end (frame 1; look 3 frame -1), a new
 * spin for looks 0, 2, 3. fx unused.
 */
void ThunderSpark_RestartFanCharge(StageObj *obj, ThunderSparkPart *part)
{
    s16 look;

    look = part->look;
    switch (look) {
    case 0:
        part->frame = 1;
        part->rot[2] = Rand_Next(1);
        break;
    case 1:
        part->frame = 1;
        break;
    case 2:
        part->frame = 1;
        part->rot[2] = Rand_Next(1);
        break;
    case 3:
        part->frame = -1;
        part->rot[2] = Rand_Next(1);
        break;
    }
}

/*
 * Type 8: restarts a part at frame 1 with a new spin (look 0 on all three
 * axes; the others z only and a scale of 1.2 to 1.8). fx unused.
 */
void ThunderSpark_RestartKirinCharge(StageObj *obj, ThunderSparkPart *part)
{
    part->frame = 1;
    switch (part->look) {
    case 0:
        part->rot[0] = Rand_Next(1);
        part->rot[1] = Rand_Next(1);
        part->rot[2] = Rand_Next(1);
        break;
    default:
        part->rot[2] = Rand_Next(1);
        part->scale = (f32) (1.2f + (0.0006f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF)));
        break;
    }
}

/*
 * Keeps the matrix's z axis and turns its x and y axes so the mesh faces the
 * camera position around that axis (left alone when degenerate).
 */
void ThunderSpark_TurnToCamera(f32 *mtx, void *pos)
{
    f32 toCam[4];
    f32 axisZ[4];
    f32 axisX[4];
    f32 axisY[4];

    Vec_Sub(toCam, &gCamPos, (f32 *) pos);
    Vec_Copy(axisZ, mtx + 8);
    Vec_Cross(axisX, toCam, axisZ);
    Vec_Normalize(axisX);
    Vec_Cross(axisY, axisZ, axisX);
    if (!(Vec_Dot(axisY, axisY) <= 0.0001f)) {
        Vec_Cross(axisX, axisY, axisZ);
        Vec_Copy(mtx, axisX);
        Vec_Copy(mtx + 4, axisY);
    }
}

/*
 * Creates sparks of type k on a monster in the current area: var2 the
 * monster's kind, scale 1.
 */
void ThunderSpark_CreateOnEm(Em *em, u8 type)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_THUNDER_SPARK;
            obj->update = ThunderSpark_Update;
            obj->type = type;
            obj->owner = em;
            obj->var2 = em->type;
            obj->node = NULL;
            obj->scale = 1.0f;
        }
    }
}

/*
 * Creates sparks of type k at pos without an owner: var2 the kind whose part
 * models it draws, and a scale.
 */
void ThunderSpark_CreateAt(void *pos, u8 type, u8 emKind, f32 scale)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_THUNDER_SPARK;
        obj->update = ThunderSpark_Update;
        obj->type = type;
        obj->owner = NULL;
        obj->var2 = emKind;
        obj->node = NULL;
        obj->scale = scale;
        Vec_Copy(obj->pos, (f32 *) pos);
    }
}

/*
 * As ThunderSpark_CreateOnEm with a scale, and a lifetime in angle: frames /
 * (2 x the monster's motionSpeed), 100 when the speed is about 0 or the
 * result is over 200 (types 3 and 8).
 */
void ThunderSpark_CreateTimed(Em *em, u8 type, s32 frames, f32 scale)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_THUNDER_SPARK;
            obj->update = ThunderSpark_Update;
            obj->type = type;
            obj->owner = em;
            obj->var2 = em->type;
            obj->node = NULL;
            obj->scale = scale;
            if (em->motionSpeed < 0.001f) {
                frames = 100;
            } else {
                frames = frames / (2.0f * em->motionSpeed);
                if (frames > 200) {
                    frames = 100;
                }
            }
            obj->angle = frames;
        }
    }
}

/*
 * The update: switch on state (0 Setup, 1 Run, 2 End, 3 Free).
 */
void WeaponTrail_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        WeaponTrail_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        WeaponTrail_Run(obj);
        break;
    case STAGEOBJ_END:
        WeaponTrail_End(obj);
        break;
    case STAGEOBJ_FREE:
        WeaponTrail_Free(obj);
        break;
    }
}

/*
 * Allocates a draw slot per trail (WeaponTrail_Counts[type]; DrawSlotB when
 * keep), fills all six history matrices with the blade matrix (bone matrix
 * plus blade offset), then runs WeaponTrail_Run once.
 */
void WeaponTrail_Setup(StageObj *obj)
{
    WeaponTrailBuf *buf = (WeaponTrailBuf *) obj->buf;
    Em *em = obj->owner;
    s32 count;
    s16 i;
    s16 j;
    f32 mtx[16];
    f32 ofs[3];

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = 0;
    count = WeaponTrail_Counts[obj->type];
    for (i = 0; i < count; i++) {
        if (obj->keep == 1) {
            buf->slot[i] = DrawSlotB_Alloc();
        } else {
            buf->slot[i] = DrawSlot_Alloc();
        }
        if (buf->slot[i] != -1) {
            if (obj->keep == 1) {
                buf->node[i] = DrawSlotB_Get(buf->slot[i]);
            } else {
                buf->node[i] = DrawSlot_Get(buf->slot[i]);
            }
            buf->node[i]->owner = obj;
            buf->node[i]->part = i;
            buf->node[i]->draw = WeaponTrail_Draw;
        } else {
            buf->node[i] = NULL;
        }
        WeaponTrail_GetBladeOffset(ofs, obj, em, i);
        WeaponTrail_GetBladeMatrix(mtx, obj, em, i);
        Vec_RotateSelf(ofs, mtx);
        mtx[12] += ofs[0];
        mtx[13] += ofs[1];
        mtx[14] += ofs[2];
        for (j = 0; j < 6; j++) {
            Mtx_Copy(buf->hist[j][i], mtx);
        }
    }
    WeaponTrail_Run(obj);
}

/*
 * Ends the trail when the owner is gone, its life runs out or it leaves both
 * motions; else shifts the history, stores this frame's blade matrix and
 * queues each trail (gDrawOtSorted) if the owner is visible and in the
 * current area. The demon trail (life -1) shows only in demon mode
 * (PL_STATUS_ICON_DEMON) or until frame 22 of PL_MOT_DSWORDS_DEMON_LEAVE, not
 * before frame 32 of PL_MOT_DSWORDS_DEMON_ENTER, nor in PL_MOT_OFFER_ITEM/_END,
 * PL_GROUP_REACT or PL_GROUP_GESTURE.
 */
void WeaponTrail_Run(StageObj *obj)
{
    Em *em = obj->owner;
    WeaponTrailBuf *buf = (WeaponTrailBuf *) obj->buf;
    s32 count;
    s16 i;
    s16 j;
    f32 mtx[16];
    f32 ofs[3];

    if (em->active == 0) {
        obj->visible = 0;
        obj->state++;
        return;
    }
    if (obj->timer != WTRAIL_LIFE_FOREVER) {
        if (--obj->timer <= 0 || (em->motionId[0] != (u16) obj->angle && em->motionId[0] != obj->angle2)) {
            obj->visible = 0;
            obj->state++;
            return;
        }
    }
    count = WeaponTrail_Counts[obj->type];
    for (i = 0; i < count; i++) {
        for (j = 5; j > 0; j--) {
            Mtx_Copy(buf->hist[j][i], buf->hist[j - 1][i]);
        }
        WeaponTrail_GetBladeOffset(ofs, obj, em, i);
        WeaponTrail_GetBladeMatrix(mtx, obj, em, i);
        Vec_RotateSelf(ofs, mtx);
        mtx[12] += ofs[0];
        mtx[13] += ofs[1];
        mtx[14] += ofs[2];
        Mtx_Copy(buf->hist[0][i], mtx);
        if (obj->timer == WTRAIL_LIFE_FOREVER) {
            if (!(em->statusIcons & PL_STATUS_ICON_DEMON)) {
                if (em->motionId[0] != PL_MOT_DSWORDS_DEMON_LEAVE || Motion_ReachedFrame(em, 0, 22.0f)) {
                    continue;
                }
            }
            if (em->motionId[0] == PL_MOT_DSWORDS_DEMON_ENTER && !Motion_ReachedFrame(em, 0, 32.0f)) {
                continue;
            }
            if ((u32) (em->motionId[0] - PL_MOT_OFFER_ITEM) <= 1 || em->stateGroup == PL_GROUP_REACT || em->stateGroup == PL_GROUP_GESTURE) {
                continue;
            }
        }
        if (((Em *)obj->owner)->visible != 0 && (Obj_InCurrentArea(obj->owner) & 0xFF) && buf->node[i] != NULL) {
            Mtx_GetTrans(buf->node[i]->pos, buf->hist[0][i]);
            OT_AddByDepth(gDrawOtSorted, buf->node[i], 0x40, 0);
        }
    }
}


/*
 * Frees the trails' draw slots (DrawSlotB when keep).
 */
void WeaponTrail_End(StageObj *obj)
{
    WeaponTrailBuf *buf = (WeaponTrailBuf *) obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = WeaponTrail_Counts[obj->type];
    for (i = 0; i < count; i++) {
        if (buf->node[i] != NULL) {
            if (obj->keep == 1) {
                DrawSlotB_Free(buf->slot[i]);
            } else {
                DrawSlot_Free(buf->slot[i]);
            }
        }
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void WeaponTrail_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/* sub_main's five common model set pointers (gCommonModels) as a named object: its load moves
   above the flags = 0 stack store (an absolute address may alias the stack store) */

extern u8 gEffectColors;
extern u8 gEffectColor0Green;
extern u8 gEffectColor0Blue;
/*
 * DrawSlot callback: sets the ribbon model's first two bones to the live
 * blade matrix and the rest to blends of the history (rotation by
 * WeaponTrail_RotWeights, position on a quadratic curve through the
 * midpoints by WeaponTrail_CurveWeights, z axis scaled to the blade's), then
 * draws matSets[WeaponTrail_MeshIdx[type]] in the type's colour: effect
 * colour 0 (type 0), white (1, 2, 4), red (3, 6), and for 5 white towards
 * red and fading in with the long sword's spirit gauge.
 */
void WeaponTrail_Draw(DrawSlot *node)
{
    u8 alpha;
    s32 bonesLeft;
    s32 seg;
    s32 step;
    StageObj *obj;
    Model *mdl;
    Em *em;
    ModelMat *materials;
    ModelMatSet *matSet;
    u8 red;
    u8 green;
    u8 blue;
    EmBone *bone;
    s16 gauge;
    f32 (*hist)[2][16];
    WeaponTrailBuf *buf;
    f32 bladeLen;
    s16 flags;
    f32 fade;
    f32 mtx[16];
    f32 mNew[16];
    f32 mMid[16];
    f32 mOld[16];
    f32 pos[3];

    obj = node->owner;
    mdl = gCommonModels[4];
    flags = 0;
    buf = (WeaponTrailBuf *) obj->buf;
    em = obj->owner;
    if (mdl == NULL || em->visible == 0) {
        return;
    } else if (mdl->active != 0) {
        bone = mdl->bones;
        bonesLeft = bone->count;
        WeaponTrail_GetBladeOffset(pos, obj, em, (s16) node->part);
        WeaponTrail_GetBladeMatrix(mtx, obj, em, (s16) node->part);
        Vec_RotateSelf(pos, mtx);
        mtx[12] += pos[0];
        mtx[13] += pos[1];
        mtx[14] += pos[2];
        Mtx_Copy(bone->world, mtx);
        Mtx_Copy(bone[1].world, mtx);
        bone += 2;
        bonesLeft -= 2;
        hist = (void *) buf->hist[0][node->part];
        /* the length of the z axis (mtx[8] to mtx[10]); written as a byte offset, the address is computed here apart from its later uses, as in the game's code */
        bladeLen = Vec_Length((f32 *) ((u8 *) mtx + 0x20));
        for (seg = 0; seg < 4; seg++) {
            Mtx_Copy(mNew, hist[seg][0]);
            Mtx_Copy(mMid, hist[seg + 1][0]);
            Mtx_Copy(mOld, hist[seg + 2][0]);
            for (step = 0; step < 6; step++) {
                Mtx_Blend(mtx, mNew, mMid, WeaponTrail_RotWeights[step][0], WeaponTrail_RotWeights[step][1]);
                mtx[8] *= bladeLen;
                mtx[9] *= bladeLen;
                mtx[10] *= bladeLen;
                Mtx_SetTrans(mtx,
                    mMid[12] * WeaponTrail_CurveWeights[step][1] + (mNew[12] + mMid[12]) / 2.0f * WeaponTrail_CurveWeights[step][0] + (mMid[12] + mOld[12]) / 2.0f * WeaponTrail_CurveWeights[step][2],
                    mMid[13] * WeaponTrail_CurveWeights[step][1] + (mNew[13] + mMid[13]) / 2.0f * WeaponTrail_CurveWeights[step][0] + (mMid[13] + mOld[13]) / 2.0f * WeaponTrail_CurveWeights[step][2],
                    mMid[14] * WeaponTrail_CurveWeights[step][1] + (mNew[14] + mMid[14]) / 2.0f * WeaponTrail_CurveWeights[step][0] + (mMid[14] + mOld[14]) / 2.0f * WeaponTrail_CurveWeights[step][2]);
                Mtx_Copy(bone->world, mtx);
                bonesLeft--;
                bone++;
                if (bonesLeft <= 0) {
                    goto done;
                }
            }
        }
    done:
        Bone_StoreMatrices(mdl->bones);
        Gfx_SetTexFilter(1);
        matSet = &mdl->matSets[WeaponTrail_MeshIdx[obj->type]];
        materials = mdl->mats;
        switch (obj->type) {
        default:
            red = gEffectColors;
            green = gEffectColor0Green;
            blue = gEffectColor0Blue;
            alpha = 0xFF;
            break;
        case WTRAIL_BLADE:
        case WTRAIL_BLADE_B:
        case WTRAIL_BOW:
            red = 0xFF;
            green = 0xFF;
            blue = 0xFF;
            alpha = 0xFF;
            flags |= 2;
            break;
        case WTRAIL_DEMON:
            red = 0xFF;
            green = 0x33;
            alpha = 0xFF;
            blue = 0x33;
            flags |= 2;
            break;
        case WTRAIL_LONG_SWORD:
            gauge = *(s16 *) &em->scriptCallEntry;
            flags |= 2;
            if (gauge < 60) {
                fade = 0.0f;
            } else {
                fade = (gauge - 60) / 60.0f;
            }
            if (fade > 1.0f) {
                fade = 1.0f;
            }
            red = 0xFF;
            green = 0xFF - (u8) (204.0f * fade);
            blue = green;
            if (gauge < 20) {
                fade = 0.0f;
            } else {
                fade = (gauge - 20) / 20.0f;
            }
            if (fade > 1.0f) {
                fade = 1.0f;
            }
            alpha = 255.0f * fade;
            break;
        case WTRAIL_LONG_SWORD_SPIRIT:
            red = 0xFF;
            green = 0x33;
            alpha = 0xFF;
            blue = 0x33;
            flags |= 2;
            break;
        }
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x67, (alpha << 24) | (red << 16) | (green << 8) | blue);
        if (matSet != NULL && matSet->id != -1) {
            Gs_SetLightList(materials, matSet);
            Draw_ApplyStates(matSet->flags);
            Draw_SetBlendFlags((u16) flags);
            Model_Draw(matSet->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Writes the weapon bone's matrix for trail "part": dual swords: bone 0xE
 * (type 1, or type 3 part 0) or bone 0x12 turned 180 degrees about Z, z axis
 * scaled by the sword's DualScales; others: horn bone 0x12 turned about Y (z
 * x1.5), long sword bone 0xE (z x2.1), else bone 0xE or 0x12 by
 * Player.unk417.
 */
void WeaponTrail_GetBladeMatrix(f32 *mtx, StageObj *obj, Em *em, s16 part)
{
    s32 sword;
    f32 xyScale;
    f32 zScale;

    sword = Player_DualSwordIndex(em);
    if (sword != -1) {
        if (obj->type == WTRAIL_BLADE || (obj->type == WTRAIL_DEMON && part == 0)) {
            xyScale = 1.0f;
            zScale = WeaponTrail_DualScalesA[sword];
            Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0xE));
        } else {
            xyScale = 1.0f;
            zScale = WeaponTrail_DualScalesB[sword];
            Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0x12));
            Mtx_TurnZ(mtx, 3.1415927f);
        }
        mtx[0] *= xyScale;
        mtx[1] *= xyScale;
        mtx[2] *= xyScale;
        mtx[4] *= xyScale;
        mtx[5] *= xyScale;
        mtx[6] *= xyScale;
        mtx[8] *= zScale;
        mtx[9] *= zScale;
        mtx[10] *= zScale;
    } else {
        switch (obj->type) {
        case WTRAIL_BOW:
            Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0x12));
            Mtx_TurnY(mtx, 3.1415927f);
            mtx[8] *= 1.5f;
            mtx[9] *= 1.5f;
            mtx[10] *= 1.5f;
            break;
        case WTRAIL_LONG_SWORD:
        case WTRAIL_LONG_SWORD_SPIRIT:
            Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0xE));
            mtx[8] *= 2.1f;
            mtx[9] *= 2.1f;
            mtx[10] *= 2.1f;
            break;
        default:
            if (em->unk417 != 0) {
                Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0xE));
            } else {
                Mtx_Copy(mtx, (f32 *) Em_GetBone(em, 0x12));
            }
            break;
        }
    }
}

/*
 * Writes the trail's offset along the blade: per dual sword index
 * (Player_DualSwordIndex) for the two dual blades, else per type.
 */
void WeaponTrail_GetBladeOffset(f32 *out, StageObj *obj, void *owner, s16 part)
{
    s32 sword;

    sword = Player_DualSwordIndex(owner);
    if (sword != -1) {
        if (obj->type == WTRAIL_BLADE || (obj->type == WTRAIL_DEMON && part == 0)) {
            out[0] = WeaponTrail_DualOffsetsA[sword].x;
            out[1] = WeaponTrail_DualOffsetsAY[sword].x;
            out[2] = WeaponTrail_DualOffsetsAZ[sword].x;
        } else {
            out[0] = WeaponTrail_DualOffsetsB[sword].x;
            out[1] = WeaponTrail_DualOffsetsBY[sword].x;
            out[2] = WeaponTrail_DualOffsetsBZ[sword].x;
        }
    } else {
        out[0] = WeaponTrail_Offsets[obj->type].x;
        out[1] = WeaponTrail_OffsetsY[obj->type].x;
        out[2] = WeaponTrail_OffsetsZ[obj->type].x;
    }
}

/*
 * Makes a trail of "type" on a player for "life" frames: type 2 not in demon
 * mode, type 5 becomes 6 at a full spirit gauge (100), type 3 in any area
 * and kept; others only in the current area. Returns the object or NULL.
 */
StageObj *WeaponTrail_Create(void *owner, s32 life, s32 type)
{
    ObjHead *head = owner;
    s16 anyArea = 0;
    StageObj *obj;

    switch ((s16) type) {
    case WTRAIL_BLADE_B:
        if (((Player *) owner)->statusIcons & PL_STATUS_ICON_DEMON) {
            return NULL;
        }
        break;
    case WTRAIL_DEMON:
        anyArea = 1;
        break;
    case WTRAIL_LONG_SWORD:
        if (((Player *) owner)->swordGauge == PL_SWORD_GAUGE_MAX) {
            type = WTRAIL_LONG_SWORD_SPIRIT;
        }
        break;
    }
    if (anyArea == 0 && !(Obj_InCurrentArea(owner) & 0xFF)) {
        return NULL;
    }
    obj = StageObj_Alloc(2);
    if (obj == NULL) {
        return NULL;
    }
    obj->kind = STAGEOBJ_KIND_WEAPON_TRAIL;
    obj->update = WeaponTrail_Update;
    obj->draw = NULL;
    obj->owner = owner;
    obj->ownerIndex = head->index;
    obj->timer = life;
    obj->angle = head->motionId[0];
    obj->type = type;
    if (anyArea) {
        obj->keep = 1;
    }
    obj->angle2 = 0;
    return obj;
}

/*
 * WeaponTrail_Create, then lets the trail last into a second motion (angle2
 * = motion2). Seen: Player_MotionEffects (sword and shield motion 0x589 with
 * 0x57E).
 */
void WeaponTrail_CreateTwoMotions(Em *pl, s32 life, s32 type, s16 motion2)
{
    StageObj *trail = WeaponTrail_Create(pl, life, type);

    if (trail != NULL) {
        trail->angle2 = motion2;
    }
}

/*
 * The update: switch on state (0 Setup, 1 Move, 2 End, 3 Delete).
 */
void EmBodyFx_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        EmBodyFx_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        EmBodyFx_Run(obj);
        break;
    case STAGEOBJ_END:
        EmBodyFx_End(obj);
        break;
    case STAGEOBJ_FREE:
        EmBodyFx_Free(obj);
        break;
    }
}

/*
 * Sets the type up: types 2 and 5 allocate three draw slots (type 5 places
 * them around the monster, EmBodyFx_RingOffsets turned by rotY, 70-frame swing,
 * scale 0.7); type 3 picks the area mesh and starts at bone 0xB; type 4
 * takes mesh 5; types 0, 1, 3, 4 allocate one slot (StageObj.slot), freeing
 * the object if none is free.
 */
void EmBodyFx_Setup(StageObj *obj)
{
    EmBodyFxBuf *buf = (EmBodyFxBuf *) obj->buf;
    Em *em = obj->owner;
    u8 i;
    f32 ofs[3];

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    obj->angle = 0;
    switch (obj->type) {
    case EMBODYFX_KIND38_CHARGE:
        for (i = 0; i < 3; i++) {
            buf->slot[i] = DrawSlot_Alloc();
            if (buf->slot[i] != -1) {
                buf->node[i] = DrawSlot_Get(buf->slot[i]);
                buf->node[i]->owner = obj;
                buf->node[i]->part = i;
                buf->node[i]->draw = EmBodyFx_Draw;
            } else {
                buf->node[i] = NULL;
            }
        }
        return;
    case EMBODYFX_SHAKALAKA_COVER:
        obj->var2 = EmBodyFx_CoverMesh(em);
        obj->angle2 = 0;
        Mtx_Copy(buf->m, Em_Bone(em, 0xB));
        Vec_Rotate(ofs, EmBodyFx_PropOffsets[obj->var2], buf->m);
        obj->pos[0] = buf->m[12] + ofs[0];
        obj->pos[1] = buf->m[13] + ofs[1];
        obj->pos[2] = buf->m[14] + ofs[2];
        break;
    case EMBODYFX_SHAKALAKA_PROP:
        obj->var2 = 5;
        break;
    case EMBODYFX_YAMATSUKAMI_RINGS:
        obj->timer = 70;
        obj->scale = 0.7f;
        obj->angle2 = 0;
        for (i = 0; i < 3; i++) {
            buf->slot[i] = DrawSlot_Alloc();
            if (buf->slot[i] != -1) {
                buf->node[i] = DrawSlot_Get(buf->slot[i]);
                buf->node[i]->owner = obj;
                buf->node[i]->part = i;
                buf->node[i]->draw = EmBodyFx_Draw;
            } else {
                buf->node[i] = NULL;
                continue;
            }
            ofs[0] = EmBodyFx_RingOffsets[i][0];
            ofs[1] = EmBodyFx_RingOffsets[i][1];
            ofs[2] = EmBodyFx_RingOffsets[i][2];
            Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
            Vec_Copy(buf->node[i]->pos, (f32 *) &em->posX);
            buf->node[i]->pos[0] += ofs[0];
            buf->node[i]->pos[1] += ofs[1];
            buf->node[i]->pos[2] += ofs[2];
        }
        return;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = EmBodyFx_Draw;
    } else {
        StageObj_Free((struct StageObj *) obj);
    }
}

/*
 * Ends when the monster is gone or being freed (taskPhase 3), else runs the
 * type: 0 Kirin's flashes while enraged (sound 0x19, 1 in 4 at bone 2 with a
 * random turn, else at bone 0x17, then a 4-19 frame wait); 1 a pulsing glow
 * at bone 0xD while enraged; 2 the charge glow at bone 2 (three parts; level
 * up while charging; shrink and flash in defeat); 3 the cover on bone 0xB,
 * thrown off when the ambush ends, two bounces to the floor, then ends; 4
 * the prop on bone 0xB; 5 the three rings around the monster.
 */
void EmBodyFx_Run(StageObj *obj)
{
    EmBodyFxBuf *buf;
    u8 i;
    Em *em;
    f32 *boneMtx;
    f32 step[3];

    em = obj->owner;
    buf = (EmBodyFxBuf *) obj->buf;
    if (em->active == 0 || em->taskPhase == 3) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    switch (obj->type) {
    case EMBODYFX_KIRIN_FLASH:
        switch (obj->var0) {
        case 0:
            if (em->rage != 0 && --obj->timer <= 0) {
                Em_PlaySeChecked(em, 0x19, 0, &em->posX, 1, 0);
                if (!(Rand_Next(1) & 3)) {
                    obj->var0 = 1;
                    obj->angle = Rand_Next(1);
                    obj->var1 = Rand_Next(1);
                    obj->var2 = Rand_Next(1);
                } else {
                    obj->var0 = 2;
                }
                obj->timer = 0;
            }
            break;
        case 1:
            if (++obj->timer >= 10) {
                obj->var0 = 0;
                obj->timer = (Rand_Next(1) & 0xF) + 4;
                break;
            }
            Em_BonePos(em, 2, obj->pos);
            Vec_Copy(obj->node->pos, (f32 *) obj->pos);
            OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
            break;
        case 2:
            if (++obj->timer >= 12) {
                obj->var0 = 0;
                obj->timer = (Rand_Next(1) & 0xF) + 4;
                break;
            }
            Em_BonePos(em, 0x17, obj->pos);
            Vec_Copy(obj->node->pos, (f32 *) obj->pos);
            OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
            break;
        }
        break;
    case EMBODYFX_KIRIN_GLOW:
        obj->timer++;
        obj->scale = 1.0f + 0.1f * Math_Sin((6.2831855f * (obj->timer << 10)) / 65536.0f);
        obj->scale *= em->scaleX;
        Em_BonePos(em, 0xD, obj->pos);
        Vec_Copy(obj->node->pos, (f32 *) obj->pos);
        if (em->rage != 0) {
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
        }
        break;
    case EMBODYFX_KIND38_CHARGE:
        obj->timer++;
        obj->var0 = EmKind38_IsCharging(em);
        switch (obj->var0) {
        case 0:
            if (--obj->angle <= 0) {
                obj->angle = 0;
            }
            break;
        case 1:
            if (++obj->angle >= 15) {
                obj->angle = 15;
            }
            break;
        }
        obj->var2 = 0;
        if (em->stateGroup == EM_GROUP_DEFEAT) {
            if (++obj->var1 < 4) {
                obj->var2 = 1;
            } else {
                obj->var1 = 4;
            }
            obj->scale -= 0.02f;
            if (obj->scale < 0.5f) {
                obj->scale = 0.5f;
            }
        } else {
            obj->scale = 1.0f;
        }
        if (em->visible != 0) {
            Em_BonePos(em, 2, obj->pos);
            for (i = 0; i < 3; i++) {
                if (buf->node[i] != NULL && (i != 2 || obj->var2 != 0)) {
                    Vec_Copy(buf->node[i]->pos, (f32 *) obj->pos);
                    OT_AddByDepth(gDrawOtSorted, buf->node[i], 0x40, 0);
                }
            }
        }
        break;
    case EMBODYFX_SHAKALAKA_COVER:
        obj->timer++;
        switch (obj->var0) {
        case 0:
            if (em->motionId[0] == 0x3F7 || (em->motionId[0] == 0x3F6 && EmMotion_ReachedFrame(em, 0, 32.0f) == 0)) {
                Mtx_Copy(buf->m, Em_Bone(em, 0xB));
                Vec_Rotate(step, EmBodyFx_PropOffsets[obj->var2], buf->m);
                obj->pos[0] = buf->m[12] + step[0];
                obj->pos[1] = buf->m[13] + step[1];
                obj->pos[2] = buf->m[14] + step[2];
            } else {
                obj->var0++;
                obj->scale = em->floorY;
                obj->angle = Rand_Next(1);
                obj->timer = 0;
            }
            break;
        case 1:
        case 2:
            obj->angle2 += 0x1000;
            if (obj->var0 < 2) {
                step[1] = 25.0f / obj->var0 - 3.5f * obj->timer;
            } else {
                step[1] = 60.0f / obj->var0 - 3.5f * obj->timer;
            }
            step[0] = 0.0f;
            step[2] = 7.5f / obj->var0;
            Vec_RotateY(step, (6.2831855f * obj->angle) / 65536.0f);
            obj->pos[0] += step[0];
            obj->pos[1] += step[1];
            obj->pos[2] += step[2];
            if (obj->pos[1] <= obj->scale) {
                obj->pos[1] = obj->scale;
                obj->var0++;
                obj->timer = 0;
            }
            break;
        case 3:
            obj->state++;
            obj->visible = 0;
            return;
        }
        Vec_Copy(obj->node->pos, (f32 *) obj->pos);
        switch (obj->var2) {
        case 3:
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
            break;
        default:
            OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
            break;
        }
        break;
    case EMBODYFX_SHAKALAKA_PROP:
        obj->timer++;
        boneMtx = Em_Bone(em, 0xB);
        Vec_Rotate(step, EmBodyFx_PropOffsets[obj->var2], boneMtx);
        obj->pos[0] = boneMtx[12] + step[0];
        obj->pos[1] = boneMtx[13] + step[1];
        obj->pos[2] = boneMtx[14] + step[2];
        Vec_Copy(obj->node->pos, (f32 *) obj->pos);
        if (em->alpha < 1.0f) {
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
        } else {
            OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
        }
        break;
    case EMBODYFX_YAMATSUKAMI_RINGS:
        switch (obj->var0) {
        case 0:
            obj->angle2 += 0x10;
            break;
        case 1:
            obj->angle2 -= 0x10;
            break;
        }
        if (obj->timer-- <= 0) {
            obj->timer = 70;
            obj->var0 = (obj->var0 + 1) & 1;
        }
        for (i = 0; i < 3; i++) {
            if (buf->node[i] != NULL) {
                buf->ofs[0] = EmBodyFx_RingOffsets[i][0];
                buf->ofs[1] = EmBodyFx_RingOffsets[i][1];
                buf->ofs[2] = EmBodyFx_RingOffsets[i][2];
                Vec_RotateY(buf->ofs, (6.2831855f * em->rotY) / 65536.0f);
                Vec_Copy(buf->node[i]->pos, (f32 *) &em->posX);
                buf->node[i]->pos[0] += buf->ofs[0];
                buf->node[i]->pos[1] += buf->ofs[1];
                buf->node[i]->pos[2] += buf->ofs[2];
                OT_AddByDepth(gDrawOt32, buf->node[i], 0x20, 0);
            }
        }
        break;
    }
}


/*
 * Frees the three slots (types 2, 5) or StageObj.slot.
 */
void EmBodyFx_End(StageObj *obj)
{
    EmBodyFxBuf *buf = (EmBodyFxBuf *) obj->buf;
    u8 i;

    obj->state++;
    switch (obj->type) {
    case EMBODYFX_KIND38_CHARGE:
        for (i = 0; i < 3; i++) {
            if (buf->node[i] != NULL) {
                DrawSlot_Free(buf->slot[i]);
            }
        }
        break;
    case EMBODYFX_YAMATSUKAMI_RINGS:
        for (i = 0; i < 3; i++) {
            if (buf->node[i] != NULL) {
                DrawSlot_Free(buf->slot[i]);
            }
        }
        break;
    default:
        DrawSlot_Free(obj->slot);
        break;
    }
}

/*
 * Frees the object (StageObj_Free). (Free would match the other families;
 * either is right.).
 */
void EmBodyFx_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * DrawSlot callback (monster in the current area): builds the type's matrix
 * and mesh (type 0: mesh var0-1 with a texture scroll, flash 2 also draws
 * mesh 8 at bone 2; 1: billboard mesh 2; 2: billboards 64/81 faded by the
 * charge level, part 2 keyed; 3, 4: mesh var2-1; 5: mesh 0 spun about Z) and
 * draws it additively (Effect_DrawModelAdd) or alpha-blended
 * (Effect_DrawModel: types 1, 2, mesh 3 of type 3, type 4 while the monster
 * fades).
 */
void EmBodyFx_Draw(DrawSlot *node)
{
    StageObj *obj;
    Model *mdl;
    ModelMatSet *matSet;
    ModelMat *materials;
    Em *em;
    u16 flags;
    s16 slotIdx;
    s16 level;
    s32 frames;
    u16 spin;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha8;
    u8 alpha;
    f32 fade;
    u32 color;
    f32 scale[3];
    f32 mtx[16];
    f32 texMtx[16];

    flags = 0;
    obj = node->owner;
    em = obj->owner;
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return;
    }
    switch (obj->type) {
    case EMBODYFX_KIRIN_FLASH:
    case EMBODYFX_KIRIN_GLOW:
    case EMBODYFX_SHAKALAKA_COVER:
    case EMBODYFX_SHAKALAKA_PROP:
    case EMBODYFX_YAMATSUKAMI_RINGS:
        slotIdx = EmSlot_FindKind(em->type);
        if (slotIdx == -1) {
            return;
        }
        mdl = gScene.partModels[slotIdx];
        break;
    case EMBODYFX_KIND38_CHARGE:
        mdl = gCommonModels[0];
        break;
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    materials = mdl->mats;
    switch (obj->type) {
    case EMBODYFX_KIRIN_FLASH:
        Mtx_Identity(mtx);
        switch (obj->var0) {
        case 1:
            frames = 10;
            Mtx_CopyRot(mtx, Em_Bone(em, 2));
            Mtx_RotateXYZ(mtx, (6.2831855f * obj->angle) / 65536.0f, (6.2831855f * (obj->var1 << 8)) / 65536.0f, (6.2831855f * (obj->var2 << 8)) / 65536.0f);
            break;
        case 2:
            frames = 12;
            matSet = &mdl->matSets[8];
            Mtx_Translation(texMtx, 0.0f, 0.1875f * (obj->timer / 12.0f), 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            Mtx_Copy(mtx, Em_Bone(em, 2));
            Gs_SetState(0x1A, (s32) mtx);
            if (matSet != NULL) {
                if (matSet->id != -1) {
                    Gs_SetLightList(materials, matSet);
                    Draw_ApplyStates(matSet->flags);
                    Model_Draw(matSet->id, 0);
                }
                Draw_SetDefaultState();
            }
            Mtx_Identity(mtx);
            Mtx_CopyRot(mtx, Em_Bone(em, 0x17));
            break;
        default:
            return;
        }
        Mtx_Translation(texMtx, 0.0f, 0.1875f * (obj->timer / (f32) frames), 0.0f);
        Gs_SetState(0x19, (s32) texMtx);
        matSet = &mdl->matSets[obj->var0] - 1;
        break;
    case EMBODYFX_KIRIN_GLOW:
        flags |= 2;
        Mtx_Scale(mtx, obj->scale, obj->scale, obj->scale);
        Mtx_Concat3x3(mtx, gCamMatrix);
        matSet = &mdl->matSets[2];
        color = 0x80FFFFFF;
        break;
    case EMBODYFX_KIND38_CHARGE:
        flags |= 2;
        switch (node->part) {
        case 0:
            level = obj->angle;
            if (level == 0) {
                fade = 1.0f;
            } else if (level == 15) {
                fade = 0.0f;
            } else {
                fade = 1.0f - level / 15.0f;
            }
            matSet = &mdl->matSets[64];
            scale[0] = scale[1] = scale[2] = EmBodyFx_ChargeBlink0[obj->timer & 1];
            alpha8 = blue = 0xFF;
            green = red = 0xA0;
            spin = 0;
            break;
        case 1:
            level = obj->angle;
            if (level == 0) {
                fade = 0.0f;
            } else if (level == 15) {
                fade = 1.0f;
            } else {
                fade = level / 15.0f;
            }
            matSet = &mdl->matSets[81];
            alpha8 = blue = green = red = 0xFF;
            scale[0] = scale[1] = scale[2] = EmBodyFx_ChargeBlink1[obj->timer & 1];
            spin = obj->timer << 11;
            break;
        case 2:
            fade = 1.0f;
            matSet = &mdl->matSets[81];
            Key_LerpVec2(EmBodyFx_FlashScaleKeys, scale, obj->var1);
            Color_EvalKeys((s32 *)EmBodyFx_FlashColorKeys, obj->var1, &color);
            spin = 0;
            green = color >> 8;
            red = color >> 16;
            blue = color;
            alpha8 = color >> 24;
            break;
        default:
            return;
        }
        scale[0] *= obj->scale * em->scaleX;
        scale[1] *= obj->scale * em->scaleY;
        scale[2] *= obj->scale * em->scaleZ;
        fade *= em->alpha;
        fade *= -1.0f + 2.0f * obj->scale;
        if (fade < 0.0f) {
            fade = 0.0f;
        }
        alpha8 = fade * alpha8;
        Mtx_Scale(mtx, scale[0], scale[1], scale[2]);
        Mtx_RotateZ(mtx, (6.2831855f * spin) / 65536.0f);
        Mtx_Concat3x3(mtx, gCamMatrix);
        color = blue | ((green << 8) | ((alpha8 << 24) | (red << 16)));
        Gs_SetState(0x6C, 0);
        break;
    case EMBODYFX_SHAKALAKA_COVER:
        if (obj->var2 == 0) {
            return;
        }
        Mtx_Identity(mtx);
        Mtx_RotateX(mtx, (6.2831855f * obj->angle2) / 65536.0f);
        Mtx_RotateY(mtx, (6.2831855f * obj->angle) / 65536.0f);
        matSet = &mdl->matSets[obj->var2] - 1;
        color = ((u8) (255.0f * em->alpha) << 24) | 0xFFFFFF;
        break;
    case EMBODYFX_SHAKALAKA_PROP:
        Mtx_Copy(mtx, Em_Bone(em, 0xB));
        matSet = &mdl->matSets[obj->var2] - 1;
        color = ((u8) (255.0f * em->alpha) << 24) | 0xFFFFFF;
        break;
    case EMBODYFX_YAMATSUKAMI_RINGS:
        flags |= 2;
        Mtx_Scale(mtx, obj->scale, obj->scale, obj->scale);
        Mtx_RotateZ(mtx, (6.2831855f * obj->angle2) / 65536.0f);
        matSet = mdl->matSets;
        color = 0xFFFFFFFF;
        break;
    }
    Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
    switch (obj->type) {
    case EMBODYFX_KIRIN_FLASH:
    case EMBODYFX_YAMATSUKAMI_RINGS:
    default:
        Effect_DrawModelAdd(matSet, mtx, materials);
        break;
    case EMBODYFX_KIRIN_GLOW:
    case EMBODYFX_KIND38_CHARGE:
        Effect_DrawModel(matSet, mtx, color, flags, materials);
        break;
    case EMBODYFX_SHAKALAKA_COVER:
        switch (obj->var2) {
        case 3:
            Effect_DrawModel(matSet, mtx, color, flags, materials);
            break;
        default:
            Effect_DrawModelAdd(matSet, mtx, materials);
            break;
        }
        break;
    case EMBODYFX_SHAKALAKA_PROP:
        if (em->alpha < 1.0f) {
            Effect_DrawModel(matSet, mtx, color, flags, materials);
        } else {
            Effect_DrawModelAdd(matSet, mtx, materials);
        }
        break;
    }
    Gs_SetState(0x6C, 1);
}

/*
 * Returns the type-3 cover's mesh number for the monster's area
 * (Area_DayId): 1 desert, 2 a forest-and-hills/jungle/swamp group, 3 a
 * snowy/jungle/swamp group, 4 volcano, 0 none (drawn as nothing).
 */
u8 EmBodyFx_CoverMesh(Em *em)
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

/*
 * Allocates a kind 7 object of "type" on em (one buffer block for types 2,
 * 3, 5); type 4 also makes a FireBlast of kind 12 (the monster's flame) on
 * em.
 */
void EmBodyFx_Spawn(Em *em, s16 type)
{
    s16 nblk;
    StageObj *obj;

    switch (type) {
    case 2:
    case 3:
    case 5:
        nblk = 1;
        break;
    case 4:
        nblk = 0;
        FireBlast_CreateOnEm(em, 0xC);
        break;
    default:
        nblk = 0;
        break;
    }
    obj = StageObj_Alloc(nblk);
    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_EM_BODY_FX;
        obj->update = EmBodyFx_Update;
        obj->owner = em;
        obj->type = type;
    }
}

/*
 * The update: switch on state (0 Setup, 1 Run, 2 End, 3 Free).
 */
void EmTail_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        EmTail_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        EmTail_Run(obj);
        break;
    case STAGEOBJ_END:
        EmTail_End(obj);
        break;
    case STAGEOBJ_FREE:
        EmTail_Free(obj);
        break;
    }
}

/*
 * Clears the phase, no carve point, type 0; keep for every kind but
 * Cephalos; allocates its draw slot (DrawSlotB when keep), freeing the
 * object if none.
 */
void EmTail_Setup(StageObj *obj)
{
    Em *em = obj->owner;

    obj->state++;
    obj->var0 = EMTAIL_WAIT;
    obj->var1 = 0;
    obj->var2 = EMTAIL_NO_POINT;
    obj->type = EMTAIL_ON_BODY;
    obj->visible = 1;
    obj->draw = 0;
    if (em->type == EM_KIND_CEPHALOS) {
        obj->keep = 0;
    } else {
        obj->keep = 1;
    }
    if (obj->keep == 1) {
        obj->slot = DrawSlotB_Alloc();
    } else {
        obj->slot = DrawSlot_Alloc();
    }
    if (obj->slot != -1) {
        if (obj->keep == 1) {
            obj->node = DrawSlotB_Get(obj->slot);
        } else {
            obj->node = DrawSlot_Get(obj->slot);
        }
        obj->node->owner = obj;
        obj->node->draw = EmTail_Draw;
    } else {
        StageObj_Free((struct StageObj *) obj);
    }
}

/*
 * Sets where the tail lies: mode 0 from the tail bone (position, heading +
 * 0x4000, and in the current area 30 above the floor probed from 100 above),
 * mode 1 the monster's position and rotY.
 */
void EmTail_SetDropPos(Em *em, StageObj *obj, s16 from)
{
    EmBone *bone;
    f32 pos[3];

    if (from == 0) {
        bone = em->model->bones;
        bone = &bone[EmTail_RootBone(em) & 0xFF];
        Mtx_GetTrans(obj->pos, bone->world);
        obj->angle = (Mtx_Heading((f32 *) bone) & 0xFFFF) + 0x4000;
        if (gScene.area == obj->angle2) {
            Em_BonePos(em, EmTail_RootBone(em) & 0xFF, pos);
            pos[1] += 100.0f;
            obj->pos[1] = 30.0f + AreaGrid2_GetFloorY(pos);
        }
    } else {
        Vec_Copy(obj->pos, &em->posX);
        obj->angle = em->rotY;
    }
}

/*
 * While on the monster (phase 0) adds the carve point at the tail
 * (EmHolder_CreateMine with the object as the holder): phase 1 with its
 * slot, or phase 2 with none; always sets type 1 (cut off).
 */
void EmTail_AddCarvePoint(StageObj *obj)
{
    s32 slot;

    if ((s32) obj->var0 <= 0) {
        slot = EmHolder_CreateMine((struct EmHolder *)obj, 1, obj->pos);
        if (slot == -1) {
            obj->var2 = EMTAIL_NO_POINT;
            obj->var0 = EMTAIL_LYING;
        } else {
            obj->var2 = (u8) slot;
            obj->var0 = EMTAIL_CARVABLE;
        }
    }
    obj->type = EMTAIL_CUT;
}

/*
 * Cuts the tail off where it is: EmTail_AddCarvePoint, then
 * EmTail_SetDropPos from the tail bone. Called by Run and
 * Em_ResetToBasePose.
 */
void EmTail_Drop(StageObj *obj)
{
    Em *em;

    em = obj->owner;
    EmTail_AddCarvePoint(obj);
    EmTail_SetDropPos(em, obj, 0);
}

/*
 * Ends (freeing the carve point) when the monster appears again (QuestEm
 * count below the one at creation, flags QUEST_EM_ACTIVE) or is gone. Phase
 * 0 waits for the kind's part-break state to drop the tail (Kushala kinds
 * and Chameleos: fly-off keys first); phase 1 keeps the point on the tail
 * and goes to 2 when carved out; then queues the draw in the tail's area
 * (gDrawOt32, gDrawOtSorted while the monster fades).
 */
void EmTail_Run(StageObj *obj)
{
    Em *em = obj->owner;
    QuestEm *questEm;

    questEm = QuestEm_FindForEm(em);
    if (questEm != NULL && obj->timer > questEm->count && questEm->flags == QUEST_EM_ACTIVE) {
        obj->state++;
        if (obj->var2 != EMTAIL_NO_POINT) {
            MinePoint_Remove(obj->var2);
        }
        return;
    }
    if (em->active == 0) {
        obj->state++;
        if (obj->var2 != EMTAIL_NO_POINT) {
            MinePoint_Remove(obj->var2);
        }
        return;
    }
    switch (obj->var0) {
    case EMTAIL_WAIT:
        obj->angle2 = em->area;
        switch (em->type) {
        case EM_KIND_RATHIAN:
        case EM_KIND_GOLD_RATHIAN:
        case EM_KIND_PINK_RATHIAN:
        case EM_KIND_RATHALOS:
        case EM_KIND_SILVER_RATHALOS:
        case EM_KIND_AZURE_RATHALOS:
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
            /* the Diablos kinds' DIAB_REACT_PART_BREAK_END has the same number */
            if (em->stateGroup == EM_GROUP_REACT && em->state == RATH_REACT_PART_BREAK_END) {
                EmTail_Drop(obj);
            }
            break;
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
        case EM_KIND_BASARIOS:
            if (em->stateGroup == EM_GROUP_REACT && em->state == GRAV_REACT_PART_BREAK_END) {
                EmTail_Drop(obj);
            }
            break;
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            switch (obj->var1) {
            case 0:
                if ((em->stateGroup == EM_GROUP_REACT && em->state == KUSH_REACT_PART_BREAK) || (em->fxFlags & 1)) {
                    obj->var1 += 2;
                    EmTail_SetDropPos(em, obj, 1);
                }
                break;
            default:
                obj->var1 += 2;
                if (obj->var1 > 0x54) {
                    EmTail_AddCarvePoint(obj);
                }
                break;
            }
            break;
        case EM_KIND_CHAMELEOS:
            switch (obj->var1) {
            case 0:
                if ((em->stateGroup == EM_GROUP_REACT && em->state == 4) || (em->fxFlags & 1)) {
                    obj->var1 += 2;
                    EmTail_SetDropPos(em, obj, 1);
                }
                break;
            default:
                obj->var1 += 2;
                if (obj->var1 > 0x54) {
                    EmTail_AddCarvePoint(obj);
                }
                break;
            }
            break;
        case EM_KIND_YIAN_GARUGA:
            if (em->stateGroup == EM_GROUP_REACT && em->state == GARUGA_REACT_PART_BREAK_END) {
                EmTail_Drop(obj);
            }
            break;
        default:
            if (em->motionId[0] == 0x429 && EmMotion_ReachedFrame(em, 0, 300.0f)) {
                EmTail_Drop(obj);
            }
            break;
        }
        break;
    case EMTAIL_CARVABLE:
        if (gScene.area == obj->angle2) {
            obj->pos[1] += 100.0f;
            obj->pos[1] = 30.0f + AreaGrid2_GetFloorY(obj->pos);
        }
        if (obj->var2 != EMTAIL_NO_POINT) {
            MinePoint_SetPos(obj->var2, obj->pos);
            if (MinePoint_Count(obj->var2) == 0) {
                obj->var0++;
                MinePoint_Remove(obj->var2);
                obj->var2 = EMTAIL_NO_POINT;
            }
        } else {
            obj->var0++;
        }
        break;
    case EMTAIL_LYING:
        if (gScene.area == obj->angle2) {
            obj->pos[1] += 100.0f;
            obj->pos[1] = 30.0f + AreaGrid2_GetFloorY(obj->pos);
        }
        break;
    }
    if (gScene.area == obj->angle2) {
        *&obj->node->pos[0] = obj->pos[0];
        obj->node->pos[1] = obj->pos[1];
        obj->node->pos[2] = obj->pos[2];
        if (1.0f == em->alpha) {
            OT_AddByDepth(gDrawOt32, obj->node, 0x20, 1);
        } else {
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 1);
        }
    }
}

/*
 * Frees the draw slot (DrawSlotB when keep).
 */
void EmTail_End(StageObj *obj)
{
    obj->state++;
    if (obj->keep == 1) {
        DrawSlotB_Free(obj->slot);
        return;
    }
    DrawSlot_Free(obj->slot);
}

/*
 * Frees the object (StageObj_Free).
 */
void EmTail_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * DrawSlot callback: unless hidden with an invisible Chameleos, poses the
 * tail skeleton (on the monster: copied from the tail bones; flying off:
 * keyed offset and rotation; cut: at pos/heading with the monster's scale),
 * stores its matrices and draws mesh 1 of the monster's model with its
 * textures.
 */
void EmTail_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    Em *em = obj->owner;
    Model *model = em->model;
    EmBone *tailBone;
    EmBone *tailBones;
    ModelMatSet *matSet;
    f32 ofs[3];
    f32 pos[3];
    f32 rot[3];
    f32 place[16];
    f32 scaleMtx[16];

    if (em->visible != 0 && model != NULL && model->active != 0) {
        if (EmTail_OwnerInvisible(em) != 1 || obj->type != EMTAIL_ON_BODY || obj->var1 != 0) {
            LightSet_SetColors(em, 1);
            LightSet_FadeColors(em);
            tailBone = em->model->bones;
            tailBone += EmTail_RootBone(em) & 0xFF;
            tailBones = em->model->partBones[1];
            if (obj->type == EMTAIL_ON_BODY) {
                if ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA || (u32) (em->type - 0x40) <= 1 || em->type == EM_KIND_CHAMELEOS) && obj->var1 != 0) {
                    Key_LerpFloat((f32 *) EmTail_FlyOfsXKeys, (f32 *) &ofs[0], obj->var1);
                    Key_LerpFloat((f32 *) EmTail_FlyOfsYKeys, (f32 *) &ofs[1], obj->var1);
                    Key_LerpFloat((f32 *) EmTail_FlyOfsZKeys, (f32 *) &ofs[2], obj->var1);
                    ofs[0] *= em->scaleX;
                    ofs[1] *= em->scaleY;
                    ofs[2] *= em->scaleZ;
                    Vec_RotateY(ofs, (6.2831855f * (u16) obj->angle) / 65536.0f);
                    Vec_Add(pos, ofs, obj->pos);
                    Mtx_Scale(scaleMtx, em->scaleX, em->scaleY, em->scaleZ);
                    Mtx_Translation(place, pos[0], pos[1], pos[2]);
                    Mtx_RotateY(place, (6.2831855f * (u16) obj->angle) / 65536.0f);
                    Mtx_Concat3x3(place, scaleMtx);
                    Key_LerpVec3((f32 *) EmTail_FlyRotKeys, (f32 *) rot, obj->var1);
                    Mtx_SetRotXYZ(tailBones[2].local, (6.2831855f * rot[0]) / 360.0f, (6.2831855f * rot[1]) / 360.0f, (6.2831855f * rot[2]) / 360.0f);
                    Bone_UpdateWorldScaledRoot(tailBones, place);
                } else {
                    Mtx_Copy(tailBones[0].world, tailBone[0].world);
                    Mtx_Copy(tailBones[1].world, tailBone[0].world);
                    Mtx_Copy(tailBones[2].world, tailBone[1].world);
                }
            } else {
                Mtx_Scale(scaleMtx, em->scaleX, em->scaleY, em->scaleZ);
                Mtx_Translation(place, obj->pos[0], obj->pos[1], obj->pos[2]);
                Mtx_RotateY(place, (6.2831855f * (u16) obj->angle) / 65536.0f);
                Mtx_Concat3x3(place, scaleMtx);
                Bone_UpdateWorldScaledRoot(tailBones, place);
            }
            Bone_StoreMatricesTo((struct MtxSlot *) gBoneMatrixBuf, tailBones);
            matSet = &model->matSets[1];
            Tex_Prepare(0xA, em->modelSlot * 20 + 0xA3);
            if (matSet->id != -1) {
                GmEm_PrepareMeshSet(em, 1, model->matSets);
                MtxSlot_LoadList((s16 *) (gModelPartLists + em->modelSlot * 0x420 + 0x42), (struct MtxSlot *) gBoneMatrixBuf);
                Draw_ApplyStates(matSet->flags);
                Model_Draw(matSet->id, 0);
            }
            Draw_SetDefaultState();
        }
    }
}

/*
 * Returns the monster's tail bone: 0x11 for the Kushala kinds (0x36, 0x3C,
 * 0x40, 0x41) and Chameleos, else 0x2B.
 */
s32 EmTail_RootBone(Em *em)
{
    switch (em->type) {
    default:
        return 0x2B;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        return 0x11;
    case EM_KIND_CHAMELEOS:
        return 0x11;
    }
}

/*
 * Returns 1 while a Chameleos is invisible (EmChameleos_IsInvisible), 0 for
 * other kinds.
 */
s32 EmTail_OwnerInvisible(Em *em)
{
    switch (em->type) {
    default:
        return 0;
    case EM_KIND_CHAMELEOS:
        return EmChameleos_IsInvisible(em) == 1;
        break;
    }
}

/*
 * Allocates the monster's tail object (STAGEOBJ_KIND_EM_TAIL), stores it in
 * em->tailObj and keeps the monster's QuestEm count in timer.
 */
void EmTail_Create(Em *em)
{
    StageObj *obj;
    QuestEm *questEm;

    if ((obj = StageObj_Alloc(0)) != NULL) {
        obj->kind = STAGEOBJ_KIND_EM_TAIL;
        obj->update = EmTail_Update;
        obj->owner = em;
        em->tailObj = obj;
        questEm = QuestEm_FindForEm(em);
        if (questEm != NULL) {
            obj->timer = questEm->count;
        }
    }
}

/*
 * The update hook: switches on state to Setup (0), Run (1), End (2), Free
 * (3).
 */
void LaoshanDust_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        LaoshanDust_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        LaoshanDust_Run(obj);
        break;
    case STAGEOBJ_END:
        LaoshanDust_End(obj);
        break;
    case STAGEOBJ_FREE:
        LaoshanDust_Free(obj);
        break;
    }
}

/*
 * Moves to Run; for type 0 takes the matrix of the bone of
 * LaoshanDust_Bones[var2]: the yaw is the bone's heading plus a quarter
 * turn, the position the bone's x/z at the monster's floorY. Sets up
 * LaoshanDust_PieceCounts[type] pieces (two): a draw slot each
 * (DrawSlot_Alloc, draw LaoshanDust_Draw, node->part = the piece), frame 0,
 * size = obj->scale, a random spin and spin speed, the object's position.
 */
void LaoshanDust_Setup(StageObj *obj)
{
    LaoshanDustPiece *piece = (LaoshanDustPiece *) obj->buf;
    Em *em = obj->owner;
    s16 count;
    s16 i;
    f32 boneMtx[16];

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    count = LaoshanDust_PieceCounts[obj->type];
    switch (obj->type) {
    case 0:
        Mtx_Copy(boneMtx, Em_Bone(em, LaoshanDust_Bones[obj->var2].bone));
        obj->angle = Mtx_Heading(boneMtx) + 0x4000;
        obj->pos[0] = boneMtx[12];
        obj->pos[1] = em->floorY;
        obj->pos[2] = boneMtx[14];
        break;
    }
    for (i = 0; i < count; i++, piece++) {
        piece->slot = DrawSlot_Alloc();
        piece->side = i;
        piece->frame = 0;
        piece->size = obj->scale;
        switch (obj->type) {
        case 0:
            piece->spin = Rand_Next(1);
            piece->spinSpeed = (Rand_Next(1) & 0xFF) - 0x80;
            Vec_Copy(piece->base, obj->pos);
            break;
        }
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = LaoshanDust_Draw;
        } else {
            piece->node = NULL;
        }
    }
}

/* Level 3 (the file is at 4: 26 words there). tbl indexed (MWCC's walker is set
   after the loop guard), and lim reused as the case's key index (a later web,
   numbered after the walker: j in s0, the walker in s1). */
#pragma optimization_level 3
/*
 * Counts timer; past LaoshanDust_Lifetimes[type] hides the object and moves
 * to End. Each piece counts its frame and, from frame 1 to its end frame,
 * reads the key tables (scale, offset, alpha) at it, adds the bone row's
 * offset, mirrors x for piece 1, turns the offset by the yaw, turns its spin
 * and queues its node at position + offset (OT_AddByDepth, gDrawOtSorted).
 * Built at optimization level 3 for the match (comment above it).
 */
void LaoshanDust_Run(StageObj *obj)
{
    LaoshanDustPiece *piece;
    s16 *pieceEnds;
    s16 count;
    s32 life;
    s16 i;
    s16 idx;
    s16 step;
    s16 end;
    f32 ofs[3];

    piece = (LaoshanDustPiece *) obj->buf;
    count = LaoshanDust_PieceCounts[obj->type];
    life = LaoshanDust_Lifetimes[obj->type];
    idx = LaoshanDust_FirstKey[obj->type];
    step = LaoshanDust_KeyStrides[obj->type];
    end = LaoshanDust_PieceEnds[obj->type];
    pieceEnds = LaoshanDust_PieceEndLists[obj->type];

    if (++obj->timer > life) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < count; i++, piece++) {
        if (pieceEnds != NULL) {
            end = pieceEnds[i];
        } else {
            idx = LaoshanDust_FirstKey[obj->type]; /* read again for each piece */
        }
        if (++piece->frame <= 0) {
            idx += step;
            continue;
        }
        if (piece->frame > end) {
            idx += step;
            continue;
        }
        switch (obj->type) {
        case 0:
            life = idx;
            Key_LerpVec3(LaoshanDust_Keys[life], piece->keyScale, piece->frame);
            Key_LerpVec3(LaoshanDust_Keys[(s16) (life + 1)], ofs, piece->frame);
            ofs[0] += LaoshanDust_Bones[obj->var2].ofs[0];
            ofs[1] += LaoshanDust_Bones[obj->var2].ofs[1];
            ofs[2] += LaoshanDust_Bones[obj->var2].ofs[2];
            if (piece->side != 0) {
                ofs[0] = -ofs[0];
            }
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            idx += 3;
            Key_LerpFloat(LaoshanDust_Keys[(s16) (life + 2)], &piece->alpha, piece->frame);
            piece->spin = piece->spin + piece->spinSpeed;
            break;
        }
        if (piece->node != NULL) {
            piece->node->pos[0] = piece->base[0] + ofs[0];
            piece->node->pos[1] = piece->base[1] + ofs[1];
            piece->node->pos[2] = piece->base[2] + ofs[2];
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
    }
}
#pragma optimization_level reset

/*
 * Moves to Free and frees the pieces' draw slots (and the object's own,
 * which it never has).
 */
void LaoshanDust_End(StageObj *obj)
{
    LaoshanDustPiece *piece = (LaoshanDustPiece *) obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = LaoshanDust_PieceCounts[obj->type];
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
    for (i = 0; i < count; i++) {
        if (piece->node != NULL) {
            DrawSlot_Free(piece->slot);
        }
        piece++;
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void LaoshanDust_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws one piece (node->part): common mesh 74 scaled by size times the
 * scale keys, turned about the view axis by its spin, facing the camera
 * (Mtx_Concat3x3 with gCamMatrix), in the area's effect colour with the
 * alpha keys' value as alpha (Effect_DrawModel). Nothing while the common
 * model set is not loaded.
 */
void LaoshanDust_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    LaoshanDustPiece *piece = (LaoshanDustPiece *) obj->buf + node->part;
    ModelMat *materials;
    ModelMatSet *model;
    Model *commonSet = gCommonModels[0];
    u32 color;
    s32 rotOrder;
    f32 mtx[16];
    f32 scale[3];
    f32 rot[3];
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;

    if (commonSet != NULL && commonSet->active != 0) {
        materials = commonSet->mats;
        Effect_GetColor(gScene.area, &red, &green, &blue, &alpha);
        scale[0] = piece->size * piece->keyScale[0];
        scale[1] = piece->size * piece->keyScale[1];
        scale[2] = piece->size * piece->keyScale[2];
        switch (obj->type) {
        case 0:
            model = &commonSet->matSets[74];
            rot[2] = (6.2831855f * piece->spin) / 65536.0f;
            rotOrder = 3;
            color = ((u8) (alpha * piece->alpha) << 24) | (red << 16) | (green << 8) | blue;
            break;
        }
        Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Effect_DrawModel(model, mtx, color, 0, materials);
    }
}

/*
 * Creates the dust for monster em when it is in the current area
 * (Player_InCurrentArea): one buffer block, kind 0xA, type k, var2 the bone
 * row c, scale f, no node.
 */
void LaoshanDust_Create(Em *em, u8 type, u8 boneRow, f32 scale)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_LAOSHAN_DUST;
            obj->update = LaoshanDust_Update;
            obj->owner = em;
            obj->type = type;
            obj->var2 = boneRow;
            obj->scale = scale;
            obj->node = NULL;
        }
    }
}

/*
 * The update hook: Setup (0), Run (1), End (2), Free (3).
 */
void PlayerHitBurst_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        PlayerHitBurst_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        PlayerHitBurst_Run(obj);
        break;
    case STAGEOBJ_END:
        PlayerHitBurst_End(obj);
        break;
    case STAGEOBJ_FREE:
        PlayerHitBurst_Free(obj);
        break;
    }
}

/*
 * Moves to Run; draws three random angles into var0, var1, var2 (Rand_Next
 * stream 1) and gets a draw slot (draw PlayerHitBurst_Draw); frees the
 * object when none is free.
 */
void PlayerHitBurst_Setup(StageObj *obj)
{

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    obj->var0 = (s32) Rand_Next(1);
    obj->var1 = (s32) Rand_Next(1);
    obj->var2 = (s32) Rand_Next(1);
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = PlayerHitBurst_Draw;
    } else {
        PlayerHitBurst_Free(obj);
        return;
    }
}

/*
 * Counts timer; at 11 moves to End; else follows the hunter's bone 10
 * (Em_GetBonePos) and queues the node (gDrawOtSorted).
 */
void PlayerHitBurst_Run(StageObj *obj)
{
    DrawSlot *node = obj->node;
    Em *pl = obj->owner;

    if (++obj->timer >= 11) {
        obj->state++;
    } else {
        Em_GetBonePos(pl, 10, obj->pos);
        Vec_Copy(node->pos, (f32 *) obj->pos);
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
    }
}

/*
 * Moves to Free and frees the draw slot.
 */
void PlayerHitBurst_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/*
 * Frees the object (StageObj_Free).
 */
void PlayerHitBurst_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws by frame (timer - 1, at least 0) when the hunter is visible in the
 * current area: frames 0-4 the flash (common mesh 66, growing, fading,
 * turned by var0), frames 2-5 and 3-6 a streak each (common mesh 0, turned
 * by var1 / var2), frames 4-9 Kushala Daora's part mesh 9 (gScene.partModels
 * of its slot) growing and turning by the three angles, all additive
 * (Gfx_SetBlend 4, 1) with depth writes off.
 */
void PlayerHitBurst_Draw(DrawSlot *node)
{
    StageObj *obj;
    ModelMat *materials;
    Model *commonSet;
    Model *kushalaSet;
    Em *pl;
    s16 frame;
    s32 burstFrame;
    s32 kushalaFrame;
    s8 kushalaSlot;
    ModelMatSet *model;
    ModelMat *kushalaMats;
    ModelMatSet *kushalaModel;
    s16 streakFrame;
    f32 flashSize;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;
    s32 pitch;
    u16 roll;
    f32 mtx[16];
    f32 uvMtx[16];

    obj = node->owner;
    pl = obj->owner;
    if (gScene.area != pl->area) {
        return;
    }
    commonSet = gCommonModels[0];
    if (commonSet == NULL || pl->visible == 0) {
        return;
    }
    if (commonSet->active == 0) {
        return;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x6C, 0);
    Gfx_SetTexFilter(1);
    frame = obj->timer - 1;
    materials = commonSet->mats;
    if (frame <= 0) {
        frame = 0;
    }
    Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
    Gs_SetState(0x19, (s32) uvMtx);
    burstFrame = (s16) frame;
    if (burstFrame < 5) {
        model = &commonSet->matSets[66];
        flashSize = 10.0f + 2.5f * frame;
        Mtx_Scale(mtx, flashSize, flashSize, 1.0f);
        Mtx_RotateZ(mtx, (6.2831855f * (obj->var0 << 8)) / 65536.0f);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, (u8) (0xEF - burstFrame * 0xB) | (((u8) (0xFF - burstFrame * 8) << 8) | (((u8) (0xFF - burstFrame * 0x3B) << 24) | 0xFF0000)));
        Gs_SetLightList(materials, model);
        Draw_ApplyStates(model->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(model->id, 0);
    }
    if (burstFrame >= 2 && burstFrame < 6) {
        model = commonSet->matSets;
        streakFrame = burstFrame - 2;
        Mtx_Scale(mtx, 0.4f + 2.5333333f * streakFrame, 4.0f - streakFrame, 1.0f);
        Mtx_RotateZ(mtx, (6.2831855f * (obj->var1 << 8)) / 65536.0f);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, 0xFFFFFFEF);
        Gs_SetLightList(materials, model);
        Draw_ApplyStates(model->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(model->id, 0);
    }
    if (burstFrame >= 3 && burstFrame < 7) {
        model = commonSet->matSets;
        streakFrame = burstFrame - 3;
        Mtx_Scale(mtx, 0.4f + 2.5333333f * streakFrame, 4.0f - streakFrame, 1.0f);
        Mtx_RotateZ(mtx, (6.2831855f * (obj->var2 << 8)) / 65536.0f);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, 0xFFFFFFEF);
        Gs_SetLightList(materials, model);
        Draw_ApplyStates(model->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(model->id, 0);
    }
    if (burstFrame >= 4 && burstFrame < 11) {
        kushalaSlot = EmSlot_FindKind(0x36);
        if (kushalaSlot == -1) {
            return;
        }
        kushalaSet = gScene.partModels[kushalaSlot];
        kushalaFrame = (s16) (burstFrame - 4);
        kushalaMats = kushalaSet->mats;
        kushalaModel = &kushalaSet->matSets[9];
        if (kushalaFrame < 3) {
            scaleY = scaleX = 0.1f + 0.10000001f * kushalaFrame;
            scaleZ = 0.7f - 0.25f * kushalaFrame;
        } else {
            scaleY = scaleX = 0.3f + 0.024999999f * kushalaFrame;
            scaleZ = 0.2f - -0.025f * kushalaFrame;
        }
        Mtx_Scale(mtx, scaleX, scaleY, scaleZ);
        roll = (u16) (obj->var0 << 6) / 6 * burstFrame;
        roll += (pitch = obj->var1 << 8);
        Mtx_RotateXYZ(mtx, (6.2831855f * pitch) / 65536.0f, (6.2831855f * (obj->var2 << 8)) / 65536.0f, (6.2831855f * roll) / 65536.0f);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, (u8) (0xEF - kushalaFrame * 7) | (((u8) (0xFF - kushalaFrame * 5) << 8) | (((u8) (0xFF - kushalaFrame * 0x27) << 24) | 0xFF0000)));
        Gs_SetLightList(kushalaMats, kushalaModel);
        Draw_ApplyStates(kushalaModel->flags);
        Gfx_SetBlend(4, 1);
        Model_Draw(kushalaModel->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(0x6C, 1);
}

/*
 * Creates the burst for hunter arg when it is in the current area
 * (Obj_InCurrentArea): no buffer, kind 0xB, owner arg.
 */
void PlayerHitBurst_Create(void *pl)
{
    StageObj *obj;

    if (Obj_InCurrentArea(pl) & 0xFF) {
        obj = StageObj_Alloc(0);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_PLAYER_HIT_BURST;
            obj->update = PlayerHitBurst_Update;
            obj->owner = pl;
        }
    }
}

/*
 * The update hook: Setup (0), Run (1), End (2), Free (3).
 */
void ItemProp_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        ItemProp_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        ItemProp_Run(obj);
        break;
    case STAGEOBJ_END:
        ItemProp_End(obj);
        break;
    case STAGEOBJ_FREE:
        ItemProp_Free(obj);
        break;
    }
}

/*
 * Moves to Run, clears var0, shows the object and calls the type's set-up
 * (0, 1, 3, 4; 5 and 6 ItemProp_SetupPuffs); frees an object of any other
 * type (type 2).
 */
void ItemProp_Setup(StageObj *obj)
{
    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    switch (obj->type) {
    case ITEMPROP_MEAT:
        ItemProp_SetupMeat(obj);
        break;
    case ITEMPROP_SMOKE:
        ItemProp_SetupSmoke(obj);
        break;
    case ITEMPROP_BAIT:
        ItemProp_SetupBait(obj);
        break;
    case ITEMPROP_DROPPED:
        ItemProp_SetupDropped(obj);
        break;
    case ITEMPROP_CLOUD:
    case ITEMPROP_EAT_PUFF:
        ItemProp_SetupPuffs(obj);
        break;
    default:
        ItemProp_Free(obj);
        break;
    }
}

/*
 * Calls the type's run (0, 1, 3, 4; 5 and 6 ItemProp_RunPuffs); nothing for
 * the others.
 */
void ItemProp_Run(StageObj *obj)
{
    switch (obj->type) {
    case ITEMPROP_MEAT:
        ItemProp_RunMeat(obj);
        break;
    case ITEMPROP_SMOKE:
        ItemProp_RunSmoke(obj);
        break;
    case ITEMPROP_BAIT:
        ItemProp_RunBait(obj);
        break;
    case ITEMPROP_DROPPED:
        ItemProp_RunDropped(obj);
        break;
    case ITEMPROP_CLOUD:
    case ITEMPROP_EAT_PUFF:
        ItemProp_RunPuffs(obj);
        
    default:
        break;
    }
}

struct DrawSlot;
/*
 * The draw slot callback of every type: calls the type's draw with the
 * object (and the node for types 1, 4, 5, 6).
 */
void ItemProp_Draw(struct DrawSlot *node)
{
    StageObj *obj;
    u8 type;

    obj = node->owner;
    type = obj->type;
    switch (type) {
    case 0:
        ItemProp_DrawMeat(obj);
        break;
    case 1:
        ItemProp_DrawSmoke(obj, node);
        break;
    case 3:
        ItemProp_DrawBait(obj);
        break;
    case 4:
        ItemProp_DrawDropped(obj, node);
        break;
    case 5:
    case 6:
        ItemProp_DrawPuff(obj, node);
        
    default:
        break;
    }
}

/*
 * Type 0: sits at the owner monster's bone 0x12, on its floorY raised 13
 * times the scale (1.75), with the owner's yaw; gets a draw slot (frees the
 * object when none) and runs the first frame. seen: no creator passes type
 * 0.
 */
void ItemProp_SetupMeat(StageObj *obj)
{
    Em *em = obj->owner;

    obj->draw = 0;
    obj->timer = 0;
    Em_GetBonePos(em, 0x12, obj->pos);
    obj->pos[1] = em->floorY;
    obj->angle = em->rotY;
    obj->scale = 1.75f;
    obj->pos[1] += 13.0f * obj->scale;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = ItemProp_Draw;
    } else {
        StageObj_Free((struct StageObj *) obj);
        return;
    }
    ItemProp_Run(obj);
}

/*
 * Type 0: after 300 frames hides the object and moves to End; else queues
 * the node at the object (gDrawOt32).
 */
void ItemProp_RunMeat(StageObj *obj)
{
    if (obj->timer++ >= 300) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    obj->node->pos[0] = obj->pos[0];
    obj->node->pos[1] = obj->pos[1];
    obj->node->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
}

/*
 * Type 0: draws common mesh 58 (the meat model: VersusBait_Draw, the
 * hunter's carried meat) scaled, 30 higher, turned by the yaw, its two
 * materials set to fixed colours.
 */
void ItemProp_DrawMeat(StageObj *obj)
{
    Model *commonSet = gCommonModels[0];
    ModelMat *materials;
    ModelMatSet *model;
    f32 *mat;
    s32 i;
    f32 mtx[16];

    if (commonSet != NULL && commonSet->active != 0) {
        Gfx_SetTexFilter(0);
        Mtx_Scale(mtx, obj->scale, obj->scale, obj->scale);
        Mtx_SetTrans(mtx, obj->pos[0], 30.0f + obj->pos[1], obj->pos[2]);
        Mtx_RotateY(mtx, (6.2831855f * (u16) obj->angle) / 65536.0f);
        Gs_SetState(0x1A, (s32) mtx);
        model = &commonSet->matSets[58];
        Gs_SetState(0x67, -1);
        if (model != NULL && model->id != -1) {
            materials = commonSet->mats;
            for (i = 0; i < model->count; i++) {
                mat = (f32 *) &materials[model->mat[i]];
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
            Draw_ApplyStates(model->flags);
            Draw_SetBlendFlags(0x10);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Type 1 (smoke screen): fills the buffer's EmObj as a sight block (radius
 * 1500 kind 1 for colour row 0, radius 3000 kind 2 for row 1, the object's
 * area and position, SightBlock_Add) and sets up four rings (draw slot, ring
 * number + 1, random spin); runs the first frame.
 */
void ItemProp_SetupSmoke(StageObj *obj)
{
    ItemPropRing *ring = (ItemPropRing *) obj->buf;
    ItemPropBuf *smoke = (ItemPropBuf *) obj->buf;
    s16 i;

    obj->draw = 0;
    obj->timer = 0;
    obj->angle = 0;
    switch (obj->var1) {
    case 0:
        smoke->obj.radius = 1500.0f;
        smoke->obj.kind = 1;
        break;
    case 1:
        smoke->obj.radius = 3000.0f;
        smoke->obj.kind = 2;
        break;
    }
    smoke->obj.timer = 3;
    smoke->obj.area = obj->angle2;
    Vec_Set(smoke->obj.pos, obj->pos[0], obj->pos[1], obj->pos[2]);
    SightBlock_Add(&smoke->obj);
    for (i = 0; i < 4; i++, ring++) {
        ring->slot = DrawSlot_Alloc();
        if (ring->slot != -1) {
            ring->view = i + 1;
            ring->unk1C = 0;
            ring->spin = Rand_Next(1);
            ring->node = DrawSlot_Get(ring->slot);
            ring->node->owner = obj;
            ring->node->part = i;
            ring->node->draw = ItemProp_Draw;
        } else {
            ring->node = NULL;
        }
    }
    ItemProp_Run(obj);
}

extern s8 ItemProp_SmokeViewQueued;
extern u8 ItemProp_SmokeViewAlpha;
/*
 * Type 1: keeps the sight block alive (timer 3 each frame) through three
 * phases in var0: 120 frames fading in, 450 opaque, 120 fading out, then
 * hides, removes the sight block and moves to End. Seen from farther than
 * 1900, queues the four rings along the view axis with spins from
 * ItemProp_SmokeRingSpins; from closer, one full-screen layer just in front
 * of the camera, whose alpha is the highest of all smokes this frame
 * (ItemProp_SmokeViewAlpha; ItemProp_SmokeViewQueued lets only the first
 * queue it).
 */
void ItemProp_RunSmoke(StageObj *obj)
{
    u8 viewAlpha;
    u32 k;
    ItemPropBuf *smoke = (ItemPropBuf *) obj->buf;
    ItemPropRing *ring = smoke->ring;
    s16 i;
    u8 opaque;
    u16 *spinStep;
    f32 camPos[3];
    f32 toCam[3];
    f32 ahead[3];
    f32 ofs[3];

    (obj->angle)++;
    smoke->obj.timer = 3;
    switch (obj->var0) {
    case 0:
        opaque = 0;
        if (++obj->timer >= 120) {
            opaque = 1;
            obj->var0++;
            obj->timer = 0;
        }
        break;
    case 1:
        opaque = 1;
        if (++obj->timer >= 450) {
            opaque = 0;
            obj->var0++;
            obj->timer = 120;
        }
        break;
    case 2:
        opaque = 0;
        if (--obj->timer <= 0) {
            obj->state++;
            obj->visible = 0;
            SightBlock_Remove(&smoke->obj);
        }
        break;
    }
    if (obj->visible != 0) {
        Mtx_GetTrans(camPos, gCamMatrix);
        Vec_Sub(toCam, camPos, obj->pos);
        if (!(Vec_Dot(toCam, toCam) <= 3610000.0f)) {
            for (i = 0, spinStep = ItemProp_SmokeRingSpins; i < 4; i++, ring++, spinStep++) {
                ring->spin += (s32) *spinStep;
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = 400.0f * i;
                Vec_RotateSelf(ofs, gCamMatrix);
                if (!(opaque & 0xFF)) {
                    ring->alpha = obj->timer / 120.0f;
                } else {
                    ring->alpha = 1.0f;
                }
                if (ring->node != NULL) {
                    ring->view = i + 1;
                    ring->node->pos[0] = obj->pos[0] + ofs[0];
                    ring->node->pos[1] = obj->pos[1] + ofs[1];
                    ring->node->pos[2] = obj->pos[2] + ofs[2];
                    OT_AddByDepth(gDrawOtSorted, ring->node, 0x40, 0);
                }
            }
        } else {
            ahead[0] = 0.0f;
            ahead[1] = 0.0f;
            ahead[2] = -1.1f;
            Vec_Rotate(ofs, ahead, gCamMatrix);
            viewAlpha = opaque == 0 ? (u32) (2.125f * obj->timer) : 0xFF;
            if (ItemProp_SmokeViewQueued == 0) {
                ItemProp_SmokeViewAlpha = viewAlpha;
                if (ring->node != NULL) {
                    ring->view = 0;
                    Mtx_GetTrans(ring->node->pos, gCamMatrix);
                    ring->node->pos[0] += ofs[0];
                    ring->node->pos[1] += ofs[1];
                    ring->node->pos[2] += ofs[2];
                    OT_AddByDepth(gDrawOtSorted, ring->node, 0x40, 0);
                    *(u8 *)&ItemProp_SmokeViewQueued = 1;
                }
            } else if (ItemProp_SmokeViewAlpha < (viewAlpha & 0xFF)) {
                ItemProp_SmokeViewAlpha = viewAlpha;
            }
        }
    }
}

/*
 * Type 1, in the object's area: the inside layer (view 0) as common mesh 67
 * with a scrolling texture and the field of view narrowed (Camera_SetFov,
 * restored after); a ring as common mesh 77 scaled by
 * ItemProp_SmokeRingScales[ring] and turned by its spin, facing the camera;
 * both in ItemProp_SmokeColors[var1] with their alpha. Clears
 * ItemProp_SmokeViewQueued.
 */
void ItemProp_DrawSmoke(StageObj *obj, DrawSlot *node)
{
    ItemPropRing *ring = (ItemPropRing *) obj->buf + node->part;
    ModelMat *materials;
    Model *commonSet = gCommonModels[0];
    ModelMatSet *model;
    u8 alpha;
    f32 mtx[16];
    f32 uvMtx[16];

    if (obj->angle2 == gScene.area && commonSet != NULL && commonSet->active != 0) {
        materials = commonSet->mats;
        Gs_SetState(0x60, 0);
        if (ring->view == 0) {
            Mtx_Scale(mtx, 1.0f, 1.0f, 1.0f);
            alpha = ItemProp_SmokeViewAlpha;
            Mtx_Translation(uvMtx, (1.0f / 2048.0f) * (gScene.frame & 0xFF), 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            model = &commonSet->matSets[67];
            Camera_SetFov(0.87266463f);
        } else {
            Mtx_Scale(mtx, ItemProp_SmokeRingScales[node->part], ItemProp_SmokeRingScales[node->part], ItemProp_SmokeRingScales[node->part]);
            Mtx_RotateZ(mtx, (6.2831855f * ring->spin) / 65536.0f);
            alpha = 255.0f * ring->alpha;
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            model = &commonSet->matSets[77];
        }
        Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, ItemProp_SmokeColorsB[obj->var1][0] | ((ItemProp_SmokeColorsG[obj->var1][0] << 8) | ((alpha << 24) | (ItemProp_SmokeColors[obj->var1][0] << 16))));
        if (model != NULL && model->id != -1) {
            Gs_SetLightList(materials, model);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        *(u8 *)&ItemProp_SmokeViewQueued = 0;
        Draw_SetDefaultState();
        if (ring->view == 0) {
            Camera_RestoreFov();
        }
    }
}

/*
 * Type 3 (a meat or bait a hunter put down): gets a draw slot from the kept
 * pool (DrawSlotB) when keep is set (the creators always set it), else the
 * normal pool; frees the object (ItemProp_Free) when none; runs the first
 * frame.
 */
void ItemProp_SetupBait(StageObj *obj)
{

    obj->var0 = 0;
    obj->draw = 0;
    obj->timer = 0;
    if (obj->keep == 0) {
        obj->slot = DrawSlot_Alloc();
    } else {
        obj->slot = DrawSlotB_Alloc();
    }
    if (obj->slot != -1) {
        if (obj->keep == 0) {
            obj->node = DrawSlot_Get(obj->slot);
        } else {
            obj->node = DrawSlotB_Get(obj->slot);
        }
        obj->node->owner = obj;
        obj->node->draw = ItemProp_Draw;
    } else {
        ItemProp_Free(obj);
        return;
    }
    ItemProp_Run(obj);
}

/*
 * Type 3: after 5400 frames, or once a monster has used it (EmObj.done),
 * hides it, takes its EmObj off the area object list (ObjList_Remove) and
 * moves to End; else queues the node (gDrawOt32).
 */
void ItemProp_RunBait(StageObj *obj)
{
    ItemPropBuf *bait = (ItemPropBuf *) obj->buf;

    obj->timer++;
    if (obj->timer > 5400 || bait->obj.done != 0) {
        obj->state++;
        obj->visible = 0;
        ObjList_Remove(&bait->obj);
        return;
    }
    obj->var0 = 0;
    obj->node->pos[0] = obj->pos[0];
    obj->node->pos[1] = obj->pos[1];
    obj->node->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
}

/*
 * Type 3, in the object's area: common mesh 58 at the object turned by its
 * yaw, material 0 coloured by ItemProp_BaitColorsFirst[var1], material 1 by
 * ItemProp_BaitColorsSecond[var1] (var1 the bait kind).
 */
void ItemProp_DrawBait(StageObj *obj)
{
    Model *commonSet = gCommonModels[0];
    ModelMatSet *model;
    ModelMat *materials;
    s32 i;
    f32 *mat;
    u8 redFirst;
    u8 greenFirst;
    u8 blueFirst;
    u8 redSecond;
    u8 greenSecond;
    u8 blueSecond;
    f32 mtx[16];

    if (obj->angle2 == gScene.area && commonSet != NULL && commonSet->active != 0) {
        model = &commonSet->matSets[58];
        Mtx_Translation(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        redSecond = ItemProp_BaitColorsSecond[obj->var1][0];
        greenSecond = ItemProp_BaitColorsSecondG[obj->var1][0];
        blueSecond = ItemProp_BaitColorsSecondB[obj->var1][0];
        redFirst = ItemProp_BaitColorsFirst[obj->var1][0];
        greenFirst = ItemProp_BaitColorsFirstG[obj->var1][0];
        blueFirst = ItemProp_BaitColorsFirstB[obj->var1][0];
        Mtx_RotateY(mtx, (6.2831855f * obj->angle) / 65536.0f);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, -1);
        if (model != NULL && model->id != -1) {
            materials = commonSet->mats;
            for (i = 0; i < model->count; i++) {
                mat = (f32 *) &materials[model->mat[i]];
                switch (i) {
                case 0:
                    mat[1] = redFirst / 255.0f;
                    mat[2] = greenFirst / 255.0f;
                    mat[3] = blueFirst / 255.0f;
                    break;
                case 1:
                    mat[1] = redSecond / 255.0f;
                    mat[2] = greenSecond / 255.0f;
                    mat[3] = blueSecond / 255.0f;
                    break;
                }
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Turns v by the bone matrix m (in place) and writes m's translation plus v
 * to out. Used by ItemProp_CreateAtPlayer for a dropped item's start.
 */
void ItemProp_OffsetFromBone(f32 *out, f32 *boneMtx, f32 *ofs)
{
    Vec_RotateSelf(ofs, boneMtx);
    Mtx_GetTrans(out, boneMtx);
    out[0] = out[0] + ofs[0];
    out[1] = out[1] + ofs[1];
    out[2] = out[2] + ofs[2];
}

/*
 * Type 4 (the item a hunter carried, let go): the node starts at the
 * object's position (the hand), the object's y becomes the owner's floorY;
 * frees the object when no draw slot; runs the first frame.
 */
void ItemProp_SetupDropped(StageObj *obj)
{

    obj->var0 = 0;
    obj->var1 = 0;
    obj->draw = 0;
    obj->timer = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = ItemProp_Draw;
        Vec_Copy(obj->node->pos, (f32 *) obj->pos);
        obj->pos[1] = ((Em *)obj->owner)->floorY;
    } else {
        StageObj_Free((struct StageObj *) obj);
        return;
    }
    ItemProp_Run(obj);
}

/*
 * Type 4: falls with growing speed (var1 frames); on reaching the floor by
 * carry kind: 0, 1, 6 land (sound 0x74 of the common bank in the current
 * area) and stay 10 frames; 2, 3, 4 and unknown kinds vanish; 5 vanishes in
 * a bomb explosion (FireBlast_CreateScaled in the current area,
 * Explosion_CreateAt EXPLOSION_BOMB). Queues the node (gDrawOt32) while it
 * lasts.
 */
void ItemProp_RunDropped(StageObj *obj)
{
    ItemPropCarry *held = (ItemPropCarry *) obj->buf;

    obj->timer++;
    obj->var1++;
    switch (obj->var0) {
    case 0:
        obj->node->pos[1] += -0.72727275f * obj->var1;
        if (obj->node->pos[1] < obj->pos[1]) {
            switch (held->kind) {
            case 0:
            case 1:
            case 6:
                obj->node->pos[1] = obj->pos[1];
                obj->timer = 0;
                obj->var0++;
                if (obj->angle2 == gScene.area) {
                    Sound_PlaySeAtPosChecked(1, 0x74, 0, obj->node->pos, 1, 0);
                }
                break;
            case 2:
            case 3:
            case 4:
            default:
                obj->state++;
                obj->visible = 0;
                return;
            case 5:
                obj->state++;
                obj->visible = 0;
                if (obj->angle2 == gScene.area) {
                    FireBlast_CreateScaled(obj->node->pos, 0, 1.0f);
                }
                Explosion_CreateAt(obj->node->pos, EXPLOSION_BOMB, obj->angle2, 0);
                return;
            }
        }
        break;
    case 1:
    default:
        if (obj->timer > 10) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        break;
    }
    OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
}

/*
 * Type 4, in the object's area: while falling, the carried model with the
 * creator's matrix (buf->mtx) at the node: common mesh 133 for kinds 0, 1, 6
 * (texture row offset) and 2, mesh 41 tinted for 3, 4, 5 (as the hunter
 * draws it carried, player_draw.c); once landed, common mesh 119 turned with
 * the camera's yaw, scale 1.5, tinted. Returns for unknown kinds.
 */
void ItemProp_DrawDropped(StageObj *obj, DrawSlot *node)
{
    Model *commonSet = gCommonModels[0];
    ItemPropCarry *held = (ItemPropCarry *) obj->buf;
    ModelMatSet *model;
    ModelMat *materials;
    s32 i;
    f32 *mat;
    u8 red;
    u8 green;
    u8 blue;
    f32 mtx[16];
    f32 uvMtx[16];

    if (obj->angle2 == gScene.area && commonSet != NULL && commonSet->active != 0) {
        switch (obj->var0) {
        case 0:
            Mtx_SetTrans(held->mtx, node->pos[0], node->pos[1], node->pos[2]);
            Mtx_Copy(mtx, held->mtx);
            switch (held->kind) {
            case 0:
            case 1:
            case 6:
                model = &commonSet->matSets[133];
                Mtx_Translation(uvMtx, 0.0f, 0.0625f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                red = 0xFF;
                green = 0xFF;
                blue = 0xFF;
                break;
            case 2:
                model = &commonSet->matSets[133];
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                red = 0xFF;
                green = 0xFF;
                blue = 0xFF;
                break;
            case 3:
                model = &commonSet->matSets[41];
                red = 0x3F;
                green = 0x3F;
                blue = 0xFF;
                break;
            case 4:
                model = &commonSet->matSets[41];
                red = 0xFF;
                green = 0x3F;
                blue = 0x3F;
                break;
            case 5:
                model = &commonSet->matSets[41];
                red = 0x3F;
                green = 0x3F;
                blue = 0x3F;
                break;
            default:
                return;
            }
            break;
        case 1:
        default:
            Mtx_Scale(mtx, 1.5f, 1.5f, 1.5f);
            Mtx_Concat3x3(mtx, gCamYawMtx);
            model = &commonSet->matSets[119];
            Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
            red = 0xEE;
            green = 0xDC;
            blue = 0xA8;
            break;
        }
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, -1);
        if (model != NULL && model->id != -1) {
            materials = commonSet->mats;
            for (i = 0; i < model->count; i++) {
                mat = (f32 *) &materials[model->mat[i]];
                switch (i) {
                case 1:
                    mat[1] = red / 255.0f;
                    mat[2] = green / 255.0f;
                    mat[3] = blue / 255.0f;
                    break;
                }
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* The file's initialised data, in the game's order */
DemonGlowPair DemonGlow_OfsLeft[32] = {
#include "game/008CCC50.inc"
};
DemonGlowPair DemonGlow_OfsRight[32] = {
#include "game/008CCD50.inc"
};
DemonGlowPair DemonGlow_ScaleLeft[32] = {
#include "game/008CCE50.inc"
};
DemonGlowPair DemonGlow_ScaleRight[31] = {
#include "game/008CCF50.inc"
};
s16 DemonGlow_BladeCounts[4] = {
#include "game/008CD048.inc"
};
ThunderSparkPoint ThunderSpark_BodyPoints[10] = {
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
u8 ThunderSpark_FanChargeColors0[32] = {
#include "game/008CD340.inc"
};
u8 ThunderSpark_FanChargeColors1[32] = {
#include "game/008CD360.inc"
};
u8 ThunderSpark_FanChargeColors2[32] = {
#include "game/008CD380.inc"
};
u8 ThunderSpark_FanChargeColors3[32] = {
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
u8 *ThunderSpark_FanBurstColors[8] = {
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
u8 *ThunderSpark_GroundBoltColors[4] = {
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
void *ThunderSpark_Keys[28] = {
    (void *)EmFxSpark_Curves_0, (void *)EmFxSpark_Curves_1, (void *)EmFxSpark_Curves_2, (void *)EmFxSpark_Curves_3,
    (void *)EmFxSpark_Curves_4, (void *)EmFxSpark_Curves_5, (void *)EmFxSpark_Curves_6, (void *)EmFxSpark_Curves_7,
    (void *)EmFxSpark_Curves_8, (void *)EmFxSpark_Curves_9, (void *)EmFxSpark_Curves_9, (void *)EmFxSpark_Curves_11,
    (void *)EmFxSpark_Curves_11, (void *)EmFxSpark_Curves_13, (void *)EmFxSpark_Curves_14, (void *)EmFxSpark_Curves_15,
    (void *)EmFxSpark_Curves_16, (void *)EmFxSpark_Curves_17, (void *)EmFxSpark_Curves_17, (void *)EmFxSpark_Curves_19,
    (void *)EmFxSpark_Curves_20, (void *)EmFxSpark_Curves_21, (void *)EmFxSpark_Curves_22, (void *)EmFxSpark_Curves_23,
    (void *)EmFxSpark_Curves_24, (void *)EmFxSpark_Curves_25, (void *)EmFxSpark_Curves_26, (void *)EmFxSpark_Curves_27,
};
s16 ThunderSpark_PartCounts[16] = {
#include "game/008CD990.inc"
};
s16 ThunderSpark_Lifetimes[16] = {
#include "game/008CD9B0.inc"
};
s16 ThunderSpark_KeyStart[16] = {
#include "game/008CD9D0.inc"
};
s16 ThunderSpark_KeyStep[16] = {
#include "game/008CD9F0.inc"
};
s16 *ThunderSpark_PartEnds[12] = {
    NULL, EmFxSpark_CurveSets_1, NULL, EmFxSpark_CurveSets_3,
    EmFxSpark_CurveSets_4, EmFxSpark_CurveSets_5, EmFxSpark_CurveSets_6, EmFxSpark_CurveSets_7,
    EmFxSpark_CurveSets_8, NULL, NULL, NULL,
};
f32 WeaponTrail_CurveWeights[6][3] = {
#include "game/008CDA40.inc"
};
f32 WeaponTrail_RotWeights[6][2] = {
#include "game/008CDA90.inc"
};
Vec3f WeaponTrail_Offsets[10] = {
#include "game/008CDAC0.inc"
};
Vec3f WeaponTrail_DualOffsetsA[32] = {
#include "game/008CDB40.inc"
};
Vec3f WeaponTrail_DualOffsetsB[32] = {
#include "game/008CDCC0.inc"
};
f32 WeaponTrail_DualScalesA[32] = {
#include "game/008CDE40.inc"
};
f32 WeaponTrail_DualScalesB[32] = {
#include "game/008CDEC0.inc"
};
f32 EmBodyFx_ChargeBlink0[2] = {
#include "game/008CDF40.inc"
};
f32 EmBodyFx_ChargeBlink1[2] = {
#include "game/008CDF48.inc"
};
f32 EmBodyFx_FlashScaleKeys[8] = {
#include "game/008CDF50.inc"
};
u8 EmBodyFx_FlashColorKeys[32] = {
#include "game/008CDF70.inc"
};
f32 EmBodyFx_PropOffsets[6][3] = {
#include "game/008CDF90.inc"
};
f32 EmBodyFx_RingOffsets[4][3] = {
#include "game/008CDFE0.inc"
};
u8 EmTail_FlyOfsXKeys[48] = {
#include "game/008CE010.inc"
};
u8 EmTail_FlyOfsYKeys[304] = {
#include "game/008CE040.inc"
};
u8 EmTail_FlyOfsZKeys[144] = {
#include "game/008CE170.inc"
};
u8 EmTail_FlyRotKeys[704] = {
#include "game/008CE200.inc"
};
u32 LaoshanDust_ScaleKeys[24] = {
#include "game/008CE4C0.inc"
};
u32 LaoshanDust_OffsetKeys[24] = {
#include "game/008CE520.inc"
};
u32 LaoshanDust_AlphaKeys[8] = {
#include "game/008CE580.inc"
};
LaoshanDustBone LaoshanDust_Bones[5] = {
#include "game/008CE5A0.inc"
};
void *LaoshanDust_Keys[4] = {
    (void *)LaoshanDust_ScaleKeys, (void *)LaoshanDust_OffsetKeys, (void *)LaoshanDust_AlphaKeys, NULL,
};
s16 LaoshanDust_PieceCounts[4] = {
#include "game/008CE600.inc"
};
s16 LaoshanDust_Lifetimes[4] = {
#include "game/008CE608.inc"
};
s16 LaoshanDust_KeyStrides[4] = {
#include "game/008CE610.inc"
};
s16 LaoshanDust_PieceEnds[4] = {
#include "game/008CE618.inc"
};
f32 ItemProp_SmokeRingScales[4] = {
#include "game/008CE620.inc"
};
u32 ItemProp_BaitStatusAmounts[2] = {
#include "game/008CE630.inc"
};
u16 ItemProp_SmokeRingSpins[4] = {
#include "game/008CE638.inc"
};
