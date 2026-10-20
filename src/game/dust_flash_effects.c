/*
 * Five kinds of stage objects drawn as effects, with parts of two more. Each
 * kind's
 *
 * update steps through four states by obj->state: set-up, run (every frame
 * while it
 *
 * lasts), end (frees its draw slots) and free; each part has a draw slot
 * whose draw
 *
 * callback runs when the depth-sorted draw tables are drawn.
 *
 * AttackBurst (kind 0x11): only its rock type, its end states and its
 * creators are here.
 *
 * ShotFlash (kind 0x12): the flash, smoke, sparks and pieces of a shot and of
 * an impact.
 *
 * AlertMark (kind 0x13): the mark over a monster. DustCloud (kind 0x14): the
 * ground
 *
 * puffs of monsters and players, and the puffs at a monster's mouth.
 * FallingDebris
 *
 * (kind 0x15), StunStars (kind 0x18), and the update and set-up of MashDebris
 * (kind 0x1B).
 *
 * Each object keeps its parts in its blocks of the object buffer (obj->buf)
 * as
 *
 * AttackBurstRock, ShotFlashPuff, ShotFlashPiece, DustCloudPuff,
 * FallingDebrisPiece or
 *
 * StunWork; most parts take their size, offset, alpha or color from key
 * curves in the
 *
 * kind's tables at the end of the file (Key_LerpVec3, Key_LerpFloat,
 * Color_EvalKeys).
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "scene.h"

/* the work block of StunStars */
typedef struct StunWork {
    s16 slot[5];                /* 0x00: a draw slot per star */
    u8 unk0A[2];
    DrawSlot *node[5];             /* 0x0C */
    f32 ofs[3];                 /* 0x20: the circle's offset from the bone; ofs[2] its radius */
    u16 group;                  /* 0x2C: the owner's state (group, state) the stars last for */
    u16 state;                  /* 0x2E */
    s16 bone;                   /* 0x30: the bone they circle */
    s16 limit;                  /* 0x32: frames (monsters) */
} StunWork;

/* a piece of a ShotFlash of the pieces type: one of two thrown off a spent bowgun shot (0x28 bytes) */
typedef struct ShotFlashPiece {
    u8 alive;                   /* 0x00 */
    u8 unk01;                   /* 0x01 */
    s16 slot;                   /* 0x02: its draw slot */
    DrawSlot *node;                /* 0x04 */
    f32 vel[3];                 /* 0x08 */
    f32 pos[3];                 /* 0x14 */
    u16 rot[3];                 /* 0x20 */
    u8 unk26[0x28 - 0x26];
} ShotFlashPiece;

/* a piece of a FallingDebris (0x34 bytes, three in its blocks) */
typedef struct FallingDebrisPiece {
    u8 alive;                   /* 0x00 */
    u8 visible;                 /* 0x01 */
    u8 phase;                   /* 0x02: 0 wait, 1 fall, 2 bounce */
    u8 unk03;
    s16 timer;                  /* 0x04 */
    s16 slot;                   /* 0x06: its draw slot, -1: none */
    f32 pos[3];                 /* 0x08 */
    f32 vel[3];                 /* 0x14 */
    s32 rot[3];                 /* 0x20 */
    DrawSlot *node;                /* 0x2C */
    f32 scale;                  /* 0x30 */
} FallingDebrisPiece;

/* a rock of an AttackBurst of the rock type (0x30 bytes, ten in its blocks) */
typedef struct AttackBurstRock {
    s16 slot;                   /* 0x00: its draw slot */
    s16 bounces;                /* 0x02: bounces left (0..2), -1: done */
    f32 vel[3];                 /* 0x04 */
    f32 pos[3];                 /* 0x10 */
    f32 scale;                  /* 0x1C */
    s16 unk20;                  /* 0x20 */
    u16 rot[3];                 /* 0x22 */
    s16 spin;                   /* 0x28: added to rot[2] each frame */
    u8 unk2A[0x2C - 0x2A];
    DrawSlot *node;                /* 0x2C */
} AttackBurstRock;

/* a puff of a ShotFlash of the puff types (0x2C bytes, after the direction matrix in its blocks) */
typedef struct ShotFlashPuff {
    f32 sc[3];                  /* 0x00: its scale (types 1 and 11: ShotFlash_FirePuffScales by pair) */
    f32 alpha;                  /* 0x0C: 1.0 at first, then from a curve */
    f32 pos[3];                 /* 0x10 */
    DrawSlot *node;                /* 0x1C */
    s16 slot;                   /* 0x20: its draw slot */
    s16 idx;                    /* 0x22: its role in the type (picks its model and curves); types 2 and 12: its spin step */
    s16 delay;                  /* 0x24: frames before it starts (negative) */
    s16 cell;                  /* 0x26: types 1 and 11: its cell of the 4x4 texture sheet, counted up each frame */
    u16 angle;                  /* 0x28 */
    s16 spin;                   /* 0x2A */
} ShotFlashPuff;

/* a puff of a DustCloud (0x30 bytes, in its blocks) */
typedef struct DustCloudPuff {
    s8 flag;                    /* 0x00: its index or look within the type; -1: gone (type 0x10, once the owner is gone) */
    u8 fixedAlpha;                   /* 0x01: nonzero: no alpha curve */
    s16 slot;                   /* 0x02: its draw slot */
    f32 sc[3];                  /* 0x04: its scale, from a curve */
    f32 pos[3];                 /* 0x10: type 0x10: its own position */
    f32 alpha;                  /* 0x1C: from a curve */
    s16 time;                   /* 0x20: frames since it started (negative: a delay) */
    u16 rot;                    /* 0x22 */
    s16 rotv;                   /* 0x24: added to rot every frame */
    u8 unk26[0x28 - 0x26];
    f32 size;                   /* 0x28: the object scale, with a random factor in some types */
    DrawSlot *node;                /* 0x2C: DrawSlot_Get(slot) */
} DustCloudPuff;

/* prototypes */
extern u8 MashDebris_PartCounts[];
extern u8 MashDebris_Models[];
extern u16 MashDebris_Lives[];
void MashDebris_Draw(void *arg);
void *memset(void *p, s32 c, u32 n);
extern s16 ShotFlash_CurveIdx[];
extern s16 ShotFlash_FirePuffDurations[];
extern s16 ShotFlash_ColorFlashDurations[];
extern s16 ShotFlash_BurstDurations[];
extern void *ShotFlash_Curves[];
extern f32 ShotFlash_FirePuffSpin[];
void ShotFlash_PlaySound(StageObj *o, f32 *pos);
void PartFx_SpawnAt(f32 *pos, s32 a, f32 f);
extern s16 ShotFlash_BurstDelays[];
extern f32 ShotFlash_FirePuffScales[];
void Vec_RotateY(f32 *v, f32 angle);
extern f32 AlertMark_MoveKeys[];
extern f32 *AlertMark_OffsetsByEmKind[];
extern f32 *AlertMark_PlayerOffset;
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
f32 StageObj_RandUnit(void);
f32 StageObj_RandSigned(void);
void AttackBurst_Draw(DrawSlot *node);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Mtx_TurnY(f32 *m, f32 a);
void FallingDebris_Draw(DrawSlot *node);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
u16 Rand_Next(s32 stream);
extern f32 AlertMark_ScaleKeys[];
extern f32 AlertMark_FixedMarkScaleKeys[];
extern s16 AlertMark_MarkMeshes[];
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Vec_RotateX(f32 *v, f32 a);
void ShotFlash_Draw(DrawSlot *node);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Gfx_SetBlend(s32 a, s32 b);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
extern void *D_008D0F34;
extern void *D_008D0F38;
/* unit's own declaration: group and state declared s32 as StunStars_Run passes them; with the definition's u8 that call compiles differently */
s16 Player_IsState(void *obj, s32 group, s32 state);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void StunStars_Draw(DrawSlot *node);
void Vec_RotateY(f32 *v, f32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Gs_SetState(u8 a, u32 b);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
extern f32 gCamMatrix[];
extern s16 DustCloud_SurfaceFx[];
extern s16 gAreaKinds[];
u8 AreaGrid2_GetFloorHeight(f32 *pos, f32 *y);
f32 Area_GroundOffset(u16 area);
void Effect08_Spawn(f32 *pos, s32 a, s32 b, f32 f);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 DustCloud_PlaceAtEm(f32 *pos, Em *em, s16 k, s16 c);
void RathalosAttack_CreateAt(Em *em, f32 *pos, s32 kind); /* unit's own declaration: kind declared s32 as the callers pass it; with the definition's s8, DustCloud_PlaceAtEm compiles differently */
s8 DustCloud_SpawnSurfaceFx(Em *em, f32 *pos, s32 mode, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void AlertMark_Draw(DrawSlot *node);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern s16 ShotFlash_Lifetimes[];
void FallingDebris_Update(StageObj *o);
void StunStars_Update(StageObj *o);
void ShotFlash_Setup(StageObj *o);
void ShotFlash_Run(StageObj *o);
void ShotFlash_End(StageObj *o);
void ShotFlash_Free(StageObj *o);
StageObj *ShotFlash_Alloc(s32 k);
void AlertMark_Setup(StageObj *o);
void AlertMark_Run(StageObj *o);
void AlertMark_End(StageObj *o);
void AlertMark_Free(StageObj *o);
void DustCloud_Setup(StageObj *o);
void DustCloud_Run(StageObj *o);
void DustCloud_End(StageObj *o);
void DustCloud_Free(StageObj *o);
void FallingDebris_Setup(StageObj *o);
void FallingDebris_Run(StageObj *o);
void FallingDebris_End(StageObj *o);
void FallingDebris_Free(StageObj *o);
void StunStars_Setup(StageObj *o);
void StunStars_Run(StageObj *o);
void StunStars_End(StageObj *o);
void StunStars_Free(StageObj *o);
void MashDebris_Setup(StageObj *o);
void MashDebris_Run(StageObj *o);
void MashDebris_End(StageObj *o);
void MashDebris_Free(StageObj *o);
s32 Obj_InCurrentArea(void *);
void ShotFlash_SetupPuffs(StageObj *o);
void ShotFlash_SetupSpark(StageObj *o);
void ShotFlash_SetupPieces(StageObj *o);
void ShotFlash_DrawPuff(DrawSlot *node);
void ShotFlash_DrawSpark(DrawSlot *node);
void ShotFlash_DrawPiece(DrawSlot *node);
void ShotFlash_RunPuffs(StageObj *o);
void ShotFlash_RunSpark(StageObj *o);
void ShotFlash_RunPieces(StageObj *o);
void Vec_Copy(f32 *dst, f32 *src);
void DrawSlot_Free(s32 i);
void AttackBurst_Update(StageObj *o);
f32 Vec_Distance(f32 *a, f32 *b);
extern s16 DustCloud_PartCounts[];
extern s16 AttackBurst_PartCounts[];
extern s16 ShotFlash_PartCounts[];
extern u8 ShotFlash_GunlanceShellDelays[];
u8 Player_InCurrentArea(void *obj);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_Billboard(f32 *m, f32 *pos);
s32 Gs_SetState(u8 a, u32 b);
extern u8 **ShotFlash_ColorFlashKeys[];
extern u8 *ShotFlash_ColorSmokeKeys[];
extern u8 *ShotFlash_ImpactKeys[];
extern u8 *ShotFlash_BurstKeys[];
extern u16 DustCloud_Lifetimes[];
extern u16 *DustCloud_PuffDurations[];
extern s16 DustCloud_CurvesPerPuff[];
extern s16 DustCloud_CurveIdx[];
extern void *DustCloud_Curves[];
extern f32 DustCloud_SideOffsets[];
extern s16 DustCloud_PuffSectors[];
u16 Mtx_Heading(f32 *m);
void FatalisAttack_CreateOnObj(StageObj *s, s8 kind);
void RathalosAttack_CreateOnObj(StageObj *s, s8 kind);
s32 Em_TypeUsesK14(u8 type);
extern s16 *DustCloud_StartTimes[];
extern f32 DustCloud_PuffSizes[];
void DustCloud_Draw(DrawSlot *node);
/* end of prototypes */

/*
 * Sets up an AttackBurst of the rock type (type 8): ten rocks, each with a
 * draw slot, 0 to 2 bounces, a random size (1.5 times obj->scale, times 1
 * plus a random unit), a random spin, and a throw speed: sideways a random
 * signed 40 (the first five) or 80 (the others) times the scale, upward 100
 * plus a random 70 times the scale, divided by the rock's size. A rock gets
 * no draw slot when none is free.
 */
void AttackBurst_SetupRocks(StageObj *obj)
{
    AttackBurstRock *rock = (AttackBurstRock *) obj->buf;
    s16 i;
    f32 riseRange;

    switch (obj->type) {
    case 8:
        riseRange = 70.0f;
        break;
    }
    obj->timer = 0;
    for (i = 0; i < 10; i++, rock++) {
        rock->slot = DrawSlot_Alloc();
        if (rock->slot != -1) {
            rock->bounces = Rand_Next(1) % 3;
            rock->unk20 = 0;
            Vec_Copy(rock->pos, obj->pos);
            rock->scale = obj->scale * (1.5f * (1.0f + StageObj_RandUnit()));
            if (i < 5) {
                rock->vel[0] = 40.0f * obj->scale * StageObj_RandSigned();
                rock->vel[1] = (obj->scale * (100.0f + riseRange * StageObj_RandUnit())) / rock->scale;
                rock->vel[2] = 40.0f * obj->scale * StageObj_RandSigned();
            } else {
                rock->vel[0] = 80.0f * obj->scale * StageObj_RandSigned();
                rock->vel[1] = (obj->scale * (100.0f + riseRange * StageObj_RandUnit())) / rock->scale;
                rock->vel[2] = 80.0f * obj->scale * StageObj_RandSigned();
            }
            rock->rot[0] = Rand_Next(1);
            rock->rot[1] = Rand_Next(1);
            rock->rot[2] = Rand_Next(1);
            rock->spin = (Rand_Next(1) & 0xFFF) - 0x7FF;
            rock->node = DrawSlot_Get(rock->slot);
            rock->node->owner = obj;
            rock->node->part = i;
            rock->node->draw = AttackBurst_Draw;
        } else {
            rock->node = NULL;
        }
    }
}

/*
 * Moves the rocks of a rock-type AttackBurst: each falls (9.8 a frame),
 * spins, and bounces off the floor at half its fall speed until its bounces
 * run out, then stops; a moving rock is queued for drawing. Moves to the end
 * state after 150 frames or once every rock has stopped.
 */
void AttackBurst_RunRocks(StageObj *obj)
{
    AttackBurstRock *rock = (AttackBurstRock *) obj->buf;
    s16 i;
    s16 done;
    f32 y;

    if (++obj->timer > 150) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    done = 0;
    for (i = 0; i < 10; i++) {
        if (rock->bounces == -1) {
            rock++;
            done++;
            continue;
        }
        rock->pos[0] += rock->vel[0];
        rock->pos[1] += rock->vel[1];
        rock->pos[2] += rock->vel[2];
        rock->vel[1] += -9.8f;
        rock->rot[2] += rock->spin;
        y = AreaGrid2_GetFloorY(rock->pos);
        if (rock->pos[1] <= y) {
            rock->bounces--;
            if (rock->bounces < 0) {
                rock++;
                continue;
            }
            rock->pos[1] = y;
            rock->vel[1] *= -0.5f;
        }
        if (rock->node != NULL) {
            rock->node->pos[0] = rock->pos[0];
            rock->node->pos[1] = rock->pos[1];
            rock->node->pos[2] = rock->pos[2];
            OT_AddByDepth(gDrawOt32, rock->node, 0x20, 0);
        }
        rock++;
    }
    if (done >= 10) {
        obj->state++;
        obj->visible = 0;
    }
}

/*
 * Draw callback of one rock of a rock-type AttackBurst (node->part picks it):
 * mesh 73 of the stage model set at the rock's rotation and size.
 */
void AttackBurst_DrawRock(DrawSlot *node)
{
    f32 rot[3];
    f32 scale[3];
    f32 mtx[16];
    ModelMat *tex;
    ModelMatSet *model;
    Model *set = gStageModelSet;
    AttackBurstRock *rock = (AttackBurstRock *) ((StageObj *) node->owner)->buf + node->part;

    if (set != NULL && set->active != 0) {
        tex = set->mats;
        rot[0] = (6.2831855f * rock->rot[0]) / 65536.0f;
        rot[1] = (6.2831855f * rock->rot[1]) / 65536.0f;
        rot[2] = (6.2831855f * rock->rot[2]) / 65536.0f;
        scale[0] = rock->scale;
        scale[1] = rock->scale;
        scale[2] = rock->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Gs_SetState(0x67, -1);
        model = &set->matSets[73];
        if (model != NULL && model->id != -1) {
            Gs_SetState(0x60, 0x80);
            Gs_SetState(0x1A, (s32) mtx);
            Gs_SetLightList(tex, model);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * End state of the AttackBurst types other than the rocks: frees each part's
 * draw slot (AttackBurst_PartCounts by type; the trail has var0 parts, at
 * least one) and moves to the free state.
 */
void AttackBurst_EndParts(StageObj *obj)
{
    u8 *cursor = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = AttackBurst_PartCounts[obj->type];
    switch (obj->type) {
    case 3:
        if (obj->var0 == 0) {
            count = 1;
        } else {
            count = obj->var0;
        }
        break;
    }
    for (i = 0; i < count; i++) {
        if (*(s32 *) (cursor + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) cursor);
        }
        cursor += 0x30;
    }
}

/*
 * End state of a rock-type AttackBurst: frees the draw slots of its ten rocks
 * and moves to the free state.
 */
void AttackBurst_EndRocks(StageObj *obj)
{
    u8 *cursor = obj->buf;
    s16 i;

    obj->state++;
    for (i = 0; i < 10; i++) {
        if (*(s32 *) (cursor + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) cursor);
        }
        cursor += 0x30;
    }
}

/* Free state of an AttackBurst: frees the object. */
void AttackBurst_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Puts an AttackBurst (the type that stays on its owner) at a bone of its
 * owner, a monster: Rajang's bone 1 at an offset of (0, 180, 330), the
 * others' bone 0x25 at (0, -60, 140), the offset turned by the bone's matrix.
 */
void AttackBurst_PlaceAtBone(StageObj *obj)
{
    Em *em = obj->owner;
    void *boneMtx;
    f32 ofs[3];

    if (em->type == EM_KIND_RAJANG) {
        boneMtx = Em_Bone(em, 1);
        ofs[0] = 0.0f;
        ofs[1] = 180.0f;
        ofs[2] = 330.0f;
    } else {
        boneMtx = Em_Bone(em, 0x25);
        ofs[0] = 0.0f;
        ofs[1] = -60.0f;
        ofs[2] = 140.0f;
    }
    Vec_RotateSelf(ofs, boneMtx);
    Mtx_GetTrans(obj->pos, boneMtx);
    obj->pos[0] += ofs[0];
    obj->pos[1] += ofs[1];
    obj->pos[2] += ofs[2];
}

/*
 * Returns 1 while the owner (a monster) is active and its layer-0 motion is
 * still the one recorded at creation (angle2) or motion 0x45A; else 0.
 */
s32 AttackBurst_OwnerInMotion(StageObj *obj)
{
    Em *em = obj->owner;

    if (em->active == 0 || (em->motionId[0] != obj->angle2 && em->motionId[0] != 0x45A)) {
        return 0;
    }
    return 1;
}

/*
 * Allocates an AttackBurst (one block) of type objType with obj->angle set to
 * angleOrBone (a yaw, or the bone for the types made at one). Returns it, or
 * NULL when no object is free.
 */
StageObj *AttackBurst_Alloc(s16 objType, s16 angleOrBone)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj == NULL) {
        return NULL;
    }
    obj->kind = STAGEOBJ_KIND_ATTACK_BURST;
    obj->update = AttackBurst_Update;
    obj->type = objType;
    obj->angle = angleOrBone;
    return obj;
}

/*
 * Creates an AttackBurst of type objType at a bone of a monster, owned by it,
 * when a player is in its area. sizeCode (kept in var0) sizes types 1 and 2
 * (0: 1.0, 1: 1.5, else 0.5); type 7 is moved by AttackBurst_PlaceAtBone and
 * keeps the monster's motion (angle2) and kind (var2), at 0.3. Rathalos' and
 * Garuga's fireball states make types 1 and 2 at bone 0x24,
 * EmRangedAttack_Run type 7.
 */
void AttackBurst_CreateAtBone(Em *em, s16 bone, s16 objType, u16 sizeCode)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = AttackBurst_Alloc(objType, bone)) != NULL) {
            obj->var0 = sizeCode;
            Em_BonePos(em, bone, obj->pos);
            obj->owner = em;
            switch (objType) {
            case 1:
            case 2:
                if (sizeCode == 0) {
                    obj->scale = 1.0f;
                } else if (sizeCode == 1) {
                    obj->scale = 1.5f;
                } else {
                    obj->scale = 0.5f;
                }
                break;
            case 7:
                AttackBurst_PlaceAtBone(obj);
                obj->angle2 = em->motionId[0];
                obj->var2 = em->type;
                obj->scale = 0.3f;
                break;
            default:
                obj->scale = 1.0f;
                break;
            }
        }
    }
}

/*
 * Creates the trail type (3) of AttackBurst at pos, without owner: pitch and
 * yaw are a projectile's angles (kept in timer and angle), count its number
 * of parts (0: one) and emKind the monster kind (var2). EmRangedAttack_Run
 * makes one every frame of a projectile's flight.
 */
void AttackBurst_CreateTrail(f32 *pos, u16 pitch, u16 yaw, u8 count, u8 emKind)
{
    StageObj *obj = AttackBurst_Alloc(3, yaw);

    if (obj != NULL) {
        obj->var0 = count;
        obj->timer = pitch;
        obj->var2 = emKind;
        obj->scale = 1.0f;
        obj->owner = NULL;
        Vec_Copy(obj->pos, pos);
    }
}

/*
 * Creates an AttackBurst of type objType at pos with a yaw and a size,
 * without owner. Many monster states and attack objects make their impact
 * bursts and rocks with it.
 */
void AttackBurst_CreateAtScaled(f32 *pos, s32 yaw, s16 objType, f32 scale)
{
    StageObj *obj = AttackBurst_Alloc(objType, yaw);

    if (obj != NULL) {
        obj->scale = scale;
        obj->owner = NULL;
        Vec_Copy(obj->pos, pos);
    }
}

/*
 * Creates an AttackBurst of type objType at a bone of a monster, owned by it,
 * when a player is in its area; the high byte of tilt (an angle) goes to
 * var0, by which type 12's set-up turns it about X. Cephadrome's states make
 * type 0xC with it.
 */
void AttackBurst_CreateAtBoneTilted(Em *em, s16 bone, s16 objType, u16 tilt)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = AttackBurst_Alloc(objType, bone)) != NULL) {
            obj->var0 = tilt >> 8;
            Em_BonePos(em, bone, obj->pos);
            obj->owner = em;
            obj->scale = 1.0f;
        }
    }
}

/*
 * Creates an AttackBurst of type objType at pos, without owner, with the
 * monster kind emKind (var2) and a size.
 */
void AttackBurst_CreateAt(f32 *pos, s16 objType, u8 emKind, f32 scale)
{
    StageObj *obj;

    if ((obj = AttackBurst_Alloc(objType, 0)) != NULL) {
        obj->var2 = emKind;
        Vec_Copy(obj->pos, pos);
        obj->owner = NULL;
        obj->scale = scale;
    }
}

/* As AttackBurst_CreateAt, with a yaw. */
void AttackBurst_CreateAtYaw(f32 *pos, s16 objType, u8 emKind, s32 yaw, f32 scale)
{
    StageObj *obj;

    if ((obj = AttackBurst_Alloc(objType, yaw)) != NULL) {
        obj->var2 = emKind;
        Vec_Copy(obj->pos, pos);
        obj->owner = NULL;
        obj->scale = scale;
    }
}

/*
 * Update of a ShotFlash: runs its set-up, run, end and free states by
 * obj->state.
 */
void ShotFlash_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        ShotFlash_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        ShotFlash_Run(obj);
        break;
    case STAGEOBJ_END:
        ShotFlash_End(obj);
        break;
    case STAGEOBJ_FREE:
        ShotFlash_Free(obj);
        break;
    }
}

/*
 * Set-up state of a ShotFlash: shows it and runs the set-up of its type: the
 * spark (type 3), the pieces (type 4) or the puffs (every other type).
 */
void ShotFlash_Setup(StageObj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    default:
        ShotFlash_SetupPuffs(obj);
        break;
    case 3:
        ShotFlash_SetupSpark(obj);
        break;
    case 4:
        ShotFlash_SetupPieces(obj);
        break;
    }
}

/*
 * Run state of a ShotFlash: runs the spark's, the pieces' or the puffs' run
 * by type.
 */
void ShotFlash_Run(StageObj *obj)
{
    switch (obj->type) {
    default:
        ShotFlash_RunPuffs(obj);
        break;
    case 3:
        ShotFlash_RunSpark(obj);
        break;
    case 4:
        ShotFlash_RunPieces(obj);
        break;
    }
}

/*
 * Draw callback of a ShotFlash's draw slots: draws a spark, a piece or a puff
 * by type.
 */
void ShotFlash_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    default:
        ShotFlash_DrawPuff(node);
        break;
    case 3:
        ShotFlash_DrawSpark(node);
        break;
    case 4:
        ShotFlash_DrawPiece(node);
        break;
    }
}

/*
 * Sets up a ShotFlash of the puff types. First the direction matrix at the
 * start of its blocks: types 0, 1, 5 and 6 take the owner's bone (obj->angle)
 * turned half a turn and move the object to an offset of the bone; types 10
 * and 11 turn by the pitch in timer and the yaw in angle. Then each puff gets
 * a draw slot, its role (idx), start delay, angle and spin, and for types 7,
 * 8 and 9 its own position. Runs the first frame at once.
 */
void ShotFlash_SetupPuffs(StageObj *obj)
{
    f32 ofs[3];
    f32 turnedMtx[16];
    f32 boneMtx[16];
    u8 *dirMtx = obj->buf;
    Em *em = obj->owner;
    s16 count = ShotFlash_PartCounts[obj->type];
    ShotFlashPuff *puff = (ShotFlashPuff *) (dirMtx + 0x40);
    s16 i;
    s16 pair;
    u16 pairAngle;
    f32 *pairScale;

    switch (obj->type) {
    case 0:
    case 1:
    case 5:
    case 6:
        ofs[0] = -10.0f;
        ofs[1] = -15.0f;
        ofs[2] = 87.0f;
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, obj->angle)));
        Mtx_GetTrans(obj->pos, boneMtx);
        Mtx_Copy(turnedMtx, boneMtx);
        Mtx_TurnY(turnedMtx, 3.1415927f);
        Vec_RotateSelf(ofs, turnedMtx);
        obj->pos[0] += ofs[0];
        obj->pos[1] += ofs[1];
        obj->pos[2] += ofs[2];
        Mtx_Copy((f32 *) dirMtx, turnedMtx);
        break;
    case 10:
    case 11:
        Mtx_Identity((f32 *) dirMtx);
        Mtx_RotateX((f32 *) dirMtx, (6.2831855f * obj->timer) / 65536.0f);
        Mtx_RotateY((f32 *) dirMtx, (6.2831855f * obj->angle) / 65536.0f);
        break;
    }
    obj->timer = 0;
    for (i = 0; i < count; puff++, i++) {
        puff->slot = DrawSlot_Alloc();
        if (puff->slot != -1) {
            puff->node = DrawSlot_Get(puff->slot);
            puff->node->owner = obj;
            puff->node->part = i;
            puff->node->draw = ShotFlash_Draw;
        } else {
            puff->node = NULL;
        }
        switch (obj->type) {
        case 0:
        case 10:
            puff->idx = i;
            puff->delay = 0;
            break;
        case 1:
        case 11:
            pair = i / 2;
            puff->idx = i & 1;
            if (puff->idx == 0) {
                puff->angle = Rand_Next(1);
                pairAngle = puff->angle;
                pairScale = &ShotFlash_FirePuffScales[pair];
                puff->sc[0] = *pairScale;
                puff->sc[1] = *pairScale;
                puff->sc[2] = *pairScale;
            } else {
                puff->angle = pairAngle;
            }
            puff->delay = -4;
            puff->alpha = 1.0f;
            if (i == 2) {
                puff->cell = 1;
            } else if (i == 4) {
                puff->cell = 3;
            } else {
                puff->cell = -1;
            }
            break;
        case 2:
            puff->idx = ((30.0f * Rand_Next(1)) / 65535.0f - 15.0f) / 20.0f;
            puff->angle = Rand_Next(1);
            puff->delay = -4 - i * 3;
            break;
        case 12:
            puff->idx = ((30.0f * Rand_Next(1)) / 65535.0f - 15.0f) / 20.0f;
            puff->angle = Rand_Next(1);
            puff->delay = -(obj->var2 + i * 3);
            break;
        case 5:
            puff->idx = i;
            switch (i) {
            case 2:
                puff->delay = 0;
                puff->angle = Rand_Next(1);
                break;
            case 4:
                puff->delay = -1;
                puff->angle = Rand_Next(1);
                break;
            default:
                puff->delay = 0;
                puff->angle = 0;
                break;
            }
            break;
        case 6:
            puff->idx = i;
            puff->angle = Rand_Next(1);
            puff->delay = -5 - i * 2;
            break;
        case 7:
            if (i == 0) {
                puff->idx = 0;
                puff->angle = 0;
                puff->delay = 0;
                Vec_Copy(puff->pos, obj->pos);
            } else {
                puff->idx = i;
                puff->angle = Rand_Next(1);
                puff->spin = (Rand_Next(1) & 0x1FF) - 0x100;
                puff->delay = -6 - (i - 1) * 5;
                puff->pos[0] = obj->pos[0] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
                puff->pos[1] = obj->pos[1] + 5.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
                puff->pos[2] = obj->pos[2] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            }
            break;
        case 8:
            puff->delay = ShotFlash_BurstDelays[i];
            Vec_Copy(puff->pos, obj->pos);
            switch (i) {
            case 0:
            case 1:
                puff->idx = i;
                puff->angle = 0;
                break;
            case 2:
            case 3:
            case 4:
            default:
                puff->idx = 2;
                puff->angle = Rand_Next(1);
                break;
            }
            break;
        case 9:
            puff->idx = i + 1;
            puff->angle = Rand_Next(1);
            puff->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            puff->delay = -6 - (i - 1) * 5;
            puff->pos[0] = obj->pos[0] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            puff->pos[1] = obj->pos[1] + 5.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            puff->pos[2] = obj->pos[2] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            break;
        }
    }
    ShotFlash_Run(obj);
}

/*
 * Runs the puffs of a ShotFlash: each puff waits out its delay, then for its
 * duration takes its scale (and for some types its offset and alpha) from the
 * type's curves (ShotFlash_Curves from ShotFlash_CurveIdx) and is queued for
 * drawing. Types 2 and 6 put a starting puff at an offset of the owner's
 * bone, type 12 at another; type 7 plays its sound at frame 6, type 8 spawns
 * an Effect0D just before its end. Ends after the type's ShotFlash_Lifetimes
 * frames, type 12 also once every puff is done.
 */
void ShotFlash_RunPuffs(StageObj *obj)
{
    f32 ofs[3];
    f32 camOfs[3];
    f32 boneMtx[16];
    u8 *dirMtx = obj->buf;
    s16 count = 0;
    ShotFlashPuff *p = (ShotFlashPuff *) (dirMtx + 0x40);
    s16 puffCount = ShotFlash_PartCounts[obj->type];
    s16 curveIdx = ShotFlash_CurveIdx[obj->type];
    s16 limit = ShotFlash_Lifetimes[obj->type];
    s16 i;
    s16 dur;
    s16 step;
    s32 curveBase;
    void *curve;
    f32 shrink;

    if (++obj->timer > limit) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    switch (obj->type) {
    case 0:
    case 10:
        dur = 7;
        step = 1;
        break;
    case 2:
    case 12:
        dur = 0x14;
        step = 3;
        break;
    case 5:
        step = 1;
        break;
    case 6:
        dur = 0x11;
        step = 1;
        break;
    case 7:
        if (obj->timer == 6) {
            ShotFlash_PlaySound(obj, obj->pos);
        }
        step = 1;
        break;
    case 8:
        step = 1;
        if (obj->timer == limit - 1) {
            PartFx_SpawnAt(obj->pos, 0x1D, 1.0f);
        }
        break;
    case 9:
        dur = 0x1E;
        step = 1;
        break;
    }
    for (i = 0; i < puffCount; i++, p++) {
        switch (obj->type) {
        case 1:
        case 11:
            if (p->idx == 0) {
                step = 1;
            } else {
                step = 3;
            }
            dur = ShotFlash_FirePuffDurations[i];
            break;
        case 2:
        case 12:
            curveIdx = 0xE;
            break;
        case 5:
            dur = ShotFlash_ColorFlashDurations[i];
            break;
        case 6:
            curveIdx = 0x18;
            break;
        case 7:
            if (p->idx == 0) {
                dur = 4;
            } else {
                dur = 0x1E;
            }
            break;
        case 8:
            dur = ShotFlash_BurstDurations[i];
            break;
        }
        if (++p->delay <= 0) {
            curveIdx += step;
            continue;
        }
        if (p->delay == 1) {
            if (obj->type == 2 || obj->type == 6) {
                if (p->node != NULL) {
                    ofs[0] = -10.0f;
                    ofs[1] = -15.0f;
                    ofs[2] = 87.0f;
                    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, obj->angle)));
                    Mtx_GetTrans(p->node->pos, boneMtx);
                    Mtx_TurnY(boneMtx, 3.1415927f);
                    Vec_RotateSelf(ofs, boneMtx);
                    p->node->pos[0] += ofs[0];
                    p->node->pos[1] += ofs[1];
                    p->node->pos[2] += ofs[2];
                }
            } else if (obj->type == 12) {
                if (p->node != NULL) {
                    ofs[0] = 110.0f;
                    ofs[1] = 16.0f;
                    ofs[2] = 170.0f;
                    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, obj->angle)));
                    Mtx_GetTrans(p->node->pos, boneMtx);
                    Vec_RotateSelf(ofs, boneMtx);
                    p->node->pos[0] += ofs[0];
                    p->node->pos[1] += ofs[1];
                    p->node->pos[2] += ofs[2];
                }
            }
        } else if (p->delay > dur) {
            curveIdx += step;
            if (obj->type == 12) {
                if (++count >= puffCount) {
                    obj->state++;
                    obj->visible = 0;
                    return;
                }
            }
            continue;
        }
        switch (obj->type) {
        case 0:
        case 5:
        case 10:
            curve = ShotFlash_Curves[curveIdx++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (obj->type == 5 && p->idx == 1) {
                p->sc[0] *= 0.75f;
                p->sc[1] *= 0.75f;
                p->sc[2] *= 0.75f;
            }
            if (p->node != NULL) {
                p->node->pos[0] = obj->pos[0];
                p->node->pos[1] = obj->pos[1];
                p->node->pos[2] = obj->pos[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 1:
        case 11:
            curve = ShotFlash_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, p->delay);
            Vec_RotateSelf((f32 *) ofs, (f32 *) dirMtx);
            p->angle += (u16) (s32) (0.5f + (65536.0f * ShotFlash_FirePuffSpin[i / 2]) / 360.0f);
            if (p->idx == 0) {
                p->cell++;
            } else {
                camOfs[0] = 0.0f;
                camOfs[1] = 0.0f;
                camOfs[2] = i * -5;
                Vec_RotateSelf(camOfs, gCamMatrix);
                ofs[0] += camOfs[0];
                ofs[1] += camOfs[1];
                ofs[2] += camOfs[2];
                curveBase = curveIdx;
                curve = ShotFlash_Curves[curveBase];
                Key_LerpVec3(curve, p->sc, p->delay);
                curveIdx += 2;
                Key_LerpFloat(ShotFlash_Curves[(s16) (curveBase + 1)], &p->alpha, p->delay);
            }
            if (p->node != NULL) {
                p->node->pos[0] = obj->pos[0] + ofs[0];
                p->node->pos[1] = obj->pos[1] + ofs[1];
                p->node->pos[2] = obj->pos[2] + ofs[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 2:
        case 12:
            curveBase = curveIdx;
            p->angle += p->idx;
            Key_LerpVec3(ShotFlash_Curves[curveBase], ofs, p->delay);
            curve = ShotFlash_Curves[(s16) (curveBase + 1)];
            Key_LerpVec3(curve, p->sc, p->delay);
            curveIdx += 3;
            Key_LerpFloat(ShotFlash_Curves[(s16) (curveBase + 2)], &p->alpha, p->delay);
            if (p->node != NULL) {
                p->node->pos[1] += 0.5f;
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 6:
            p->angle += 0xA1;
            curve = ShotFlash_Curves[curveIdx++];
            Key_LerpVec3(curve, p->sc, p->delay);
            shrink = 1.4f - 0.2f * i;
            p->sc[0] *= shrink;
            p->sc[1] *= shrink;
            p->sc[2] *= shrink;
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = (100.0f - 20.0f * i) / dur;
            if (p->node != NULL) {
                Vec_RotateSelf((f32 *) ofs, (f32 *) dirMtx);
                p->node->pos[0] += ofs[0];
                p->node->pos[1] += ofs[1];
                p->node->pos[2] += ofs[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 7:
        case 9:
            curve = ShotFlash_Curves[curveIdx++];
            Key_LerpVec3(curve, p->sc, p->delay);
            p->sc[0] *= obj->scale;
            p->sc[1] *= obj->scale;
            p->sc[2] *= obj->scale;
            if (p->node != NULL) {
                if (p->idx == 0) {
                    ofs[0] = 0.0f;
                    ofs[1] = 0.0f;
                    ofs[2] = 30.0f;
                    Vec_RotateSelf(ofs, gCamMatrix);
                    p->node->pos[0] = p->pos[0] + ofs[0];
                    p->node->pos[1] = p->pos[1] + ofs[1];
                    p->node->pos[2] = p->pos[2] + ofs[2];
                } else {
                    p->angle += p->spin;
                    ofs[0] = 0.0f;
                    ofs[1] = 0.0f;
                    ofs[2] = 5.0f * i;
                    p->pos[1] += 2.0f;
                    p->node->pos[0] = p->pos[0] + ofs[0];
                    p->node->pos[1] = p->pos[1] + ofs[1];
                    p->node->pos[2] = p->pos[2] + ofs[2];
                }
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 8:
            curve = ShotFlash_Curves[curveIdx++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (p->node != NULL) {
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        }
    }
}

/*
 * Draw callback of one ShotFlash puff: picks the stage model set's mesh,
 * matrix and color by type and role: white flashes (types 0, 10), a cell of a
 * 4x4 texture sheet and fading smoke (1, 11), smoke (2, 12, the latter twice
 * as big), colors of the ammo's color set var2 (5, 6, 7, 9) or of the role
 * (8). Most face the camera; types 0 and 10's second puff and type 5's first
 * two start from the direction matrix, turned by Mtx_Billboard. Depth writes
 * are off while types 0, 1, 5, 10 and 11 draw.
 */
void ShotFlash_DrawPuff(DrawSlot *node)
{
    StageObj *obj = node->owner;
    ModelMat *mats;
    f32 *dirMtx = (f32 *) obj->buf;
    ShotFlashPuff *puff = (ShotFlashPuff *) ((u8 *) dirMtx + node->part * sizeof(ShotFlashPuff) + 0x40);
    u8 **tbl;
    s16 meshIdx;
    u16 mode = 0;
    Model *set = gStageModelSet;
    u32 color;
    f32 *scaleY;
    f32 *scaleZ;
    f32 axisScale;
    f32 rot[3];
    f32 scale[3];
    f32 mtx[16];
    f32 uvMtx[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        switch (obj->type) {
        case 0:
        case 10:
            mode |= 2;
            Gs_SetState(0x6C, 0);
            switch (puff->idx) {
            case 0:
                rot[0] = 0.0f;
                meshIdx = 0x10;
                rot[1] = 0.0f;
                rot[2] = 0.0f;
                Mtx_FromScaleRotTrans(puff->sc, rot, node->pos, 0, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            case 1:
                if (puff->delay == 4) {
                    meshIdx = 0x12;
                } else {
                    meshIdx = 0x11;
                }
                Mtx_Copy(mtx, dirMtx);
                Mtx_Billboard(mtx, obj->pos);
                mtx[0] *= puff->sc[0];
                mtx[1] *= puff->sc[0];
                mtx[2] *= puff->sc[0];
                mtx[4] *= puff->sc[1];
                mtx[5] *= puff->sc[1];
                mtx[6] *= puff->sc[1];
                mtx[8] *= puff->sc[2];
                mtx[9] *= puff->sc[2];
                mtx[10] *= puff->sc[2];
                Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
                break;
            }
            color = 0xFFFFFFFF;
            break;
        case 1:
        case 11:
            Gs_SetState(0x6C, 0);
            if (puff->idx == 0) {
                mode |= 2;
                meshIdx = 0x4E;
                Mtx_Translation(uvMtx, 0.25f * (puff->cell & 3), 0.25f * (puff->cell / 4), 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
            } else {
                meshIdx = 0x14;
            }
            rot[2] = (6.2831855f * puff->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(puff->sc, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            color = ((u8) (255.0f * puff->alpha) << 24) | 0xFFFFFF;
            break;
        case 2:
        case 12:
            meshIdx = 0x14;
            if (obj->type == 12) {
                scale[0] = 2.0f * puff->sc[0];
                scale[1] = 2.0f * puff->sc[1];
                scale[2] = 2.0f * puff->sc[2];
            } else {
                Vec_Copy(scale, puff->sc);
            }
            rot[2] = (6.2831855f * puff->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            color = ((u8) (255.0f * puff->alpha) << 24) | 0xFFFFFF;
            break;
        case 5:
            mode |= 2;
            Gs_SetState(0x6C, 0);
            scaleY = &scale[1];
            scaleZ = &scale[2];
            scale[0] = 3.0f * puff->sc[0];
            *scaleY = 3.0f * puff->sc[1];
            *scaleZ = 3.0f * puff->sc[2];
            tbl = ShotFlash_ColorFlashKeys[puff->idx];
            if (tbl == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *) tbl[obj->var2], puff->delay, &color);
            }
            switch (puff->idx) {
            case 0:
            case 1:
                meshIdx = 0x79;
                Mtx_Copy(mtx, dirMtx);
                Mtx_Billboard(mtx, obj->pos);
                mtx[0] *= scale[0];
                mtx[1] *= scale[0];
                mtx[2] *= scale[0];
                axisScale = *scaleY;
                mtx[4] *= axisScale;
                mtx[5] *= axisScale;
                mtx[6] *= axisScale;
                axisScale = *scaleZ;
                mtx[8] *= axisScale;
                mtx[9] *= axisScale;
                mtx[10] *= axisScale;
                Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
                break;
            case 2:
                meshIdx = 0x42;
                rot[2] = (6.2831855f * puff->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 3:
                meshIdx = 0x40;
                rot[2] = (6.2831855f * puff->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            case 4:
                meshIdx = 0x23;
                rot[2] = (6.2831855f * puff->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                Mtx_Concat3x3(mtx, gCamMatrix);
                break;
            }
            switch (obj->var2) {
            case 0:
            case 5:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
                mode |= 2;
                break;
            }
            break;
        case 6:
            scale[0] = 3.0f * puff->sc[0];
            scale[1] = 3.0f * puff->sc[1];
            scale[2] = 3.0f * puff->sc[2];
            meshIdx = 0x48;
            rot[2] = (6.2831855f * puff->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Color_EvalKeys((s32 *) ShotFlash_ColorSmokeKeys[obj->var2], puff->delay, &color);
            switch (obj->var2) {
            case 0:
            case 5:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
                mode |= 2;
                break;
            }
            break;
        case 7:
        case 9:
            if (puff->idx == 0) {
                meshIdx = 0x40;
                mode |= 2;
                color = 0xFFFFFF7F;
                scale[0] = 4.0f * puff->sc[0];
                scale[1] = 4.0f * puff->sc[1];
                scale[2] = 4.0f * puff->sc[2];
            } else {
                meshIdx = 0x48;
                Color_EvalKeys((s32 *) ShotFlash_ImpactKeys[obj->var2], puff->delay, &color);
                scale[0] = 2.0f * (1.0f - 0.1f * (puff->idx - 1)) * puff->sc[0];
                scale[1] = 2.0f * (1.0f - 0.1f * (puff->idx - 1)) * puff->sc[1];
                scale[2] = 2.0f * (1.0f - 0.1f * (puff->idx - 1)) * puff->sc[2];
            }
            rot[2] = (6.2831855f * puff->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 8:
            mode |= 2;
            switch (puff->idx) {
            case 0:
                meshIdx = 0x40;
                break;
            case 1:
                meshIdx = 0x58;
                break;
            case 2:
                meshIdx = 0x7A;
                break;
            }
            Color_EvalKeys((s32 *) ShotFlash_BurstKeys[puff->idx], puff->delay, &color);
            rot[2] = (6.2831855f * puff->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(puff->sc, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        }
        Effect_DrawModel(&set->matSets[meshIdx], mtx, color, mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Sets up a ShotFlash spark (type 3): a random roll and a draw slot of the
 * object's own at its position; frees the object when no draw slot is free.
 */
void ShotFlash_SetupSpark(StageObj *obj)
{
    obj->angle = Rand_Next(1);
    obj->timer = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = ShotFlash_Draw;
        Vec_Copy(obj->node->pos, obj->pos);
    } else {
        StageObj_Free(obj);
    }
}

/*
 * Runs a ShotFlash spark: queues it for drawing until the type's frame limit,
 * then moves to the end state.
 */
void ShotFlash_RunSpark(StageObj *obj)
{
    if (++obj->timer > ShotFlash_Lifetimes[obj->type]) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
}

/*
 * Draw callback of a ShotFlash spark: mesh 88 of the common model set facing
 * the camera, its size and alpha from ShotFlash_SparkScaleCurve and
 * ShotFlash_SparkAlphaCurve by its frame.
 */
void ShotFlash_DrawSpark(DrawSlot *node)
{
    f32 alpha;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    StageObj *obj = node->owner;
    ModelMat *mats;
    ModelMatSet *mesh;
    Model *mdl = gCommonModels[0];

    if (mdl != NULL && mdl->active != 0) {
        mats = mdl->mats;
        Key_LerpVec3(D_008D0F34, scale, obj->timer);
        Key_LerpFloat(D_008D0F38, &alpha, obj->timer);
        rot[2] = (6.2831855f * obj->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        mesh = &mdl->matSets[88];
        Gfx_SetBlend(4, 1);
        Effect_DrawModel(mesh, mtx, ((u8) (255.0f * alpha) << 24) | 0xFFFFFF, 2, mats);
        Gfx_SetBlend(4, 5);
    }
}

/*
 * Sets up a ShotFlash of the pieces type (type 4): two pieces at its
 * position, sent along the direction in var0 (pitch) and var1 (yaw) at speed
 * obj->scale, each pushed 10 to its own side and 3 up, the second rolled half
 * a turn. Runs the first frame at once.
 */
void ShotFlash_SetupPieces(StageObj *obj)
{
    f32 sidePush[3];
    f32 launchVel[3];
    ShotFlashPiece *piece = (ShotFlashPiece *) obj->buf;
    s16 i;
    u16 pitch;
    u16 yaw;
    s16 count = ShotFlash_PartCounts[obj->type];

    launchVel[0] = 0.0f;
    launchVel[1] = 0.0f;
    launchVel[2] = obj->scale;
    pitch = obj->var0 << 8;
    yaw = obj->var1 << 8;
    Vec_RotateX(launchVel, (6.2831855f * pitch) / 65536.0f);
    Vec_RotateY(launchVel, (6.2831855f * yaw) / 65536.0f);
    obj->timer = 0;
    for (i = 0; i < count; i++, piece++) {
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->alive = 1;
            piece->unk01 = 1;
            piece->vel[0] = launchVel[0];
            piece->vel[1] = launchVel[1];
            piece->vel[2] = launchVel[2];
            piece->rot[0] = pitch;
            piece->rot[1] = yaw;
            if (i == 0) {
                piece->rot[2] = 0;
            } else {
                piece->rot[2] = 0x8000;
            }
            Vec_Copy(piece->pos, obj->pos);
            sidePush[0] = 0.0f;
            sidePush[1] = 3.0f;
            sidePush[2] = 10.0f;
            Vec_RotateY(sidePush, (6.2831855f * (piece->rot[1] + (i ? -0x4000 : 0x4000))) / 65536.0f);
            piece->vel[0] += sidePush[0];
            piece->vel[1] += sidePush[1];
            piece->vel[2] += sidePush[2];
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = ShotFlash_Draw;
        } else {
            piece->node = NULL;
        }
    }
    ShotFlash_RunPieces(obj);
}

/*
 * Moves the pieces of a ShotFlash: each falls (2.5 a frame), slows by a
 * tenth, tumbles, and stops at the floor; a moving piece is queued for
 * drawing. Ends after the type's frame limit or when no piece moves.
 */
void ShotFlash_RunPieces(StageObj *obj)
{
    ShotFlashPiece *piece = (ShotFlashPiece *) obj->buf;
    s16 count = ShotFlash_PartCounts[obj->type];
    s16 i;
    u8 alive = 0;

    if (++obj->timer > ShotFlash_Lifetimes[obj->type]) {
        obj->state++;
        obj->visible = 0;
    }
    for (i = 0; i < count; i++) {
        if (piece->alive == 0) {
            piece++;
            continue;
        }
        alive = 1;
        piece->pos[0] += piece->vel[0];
        piece->pos[1] += piece->vel[1];
        piece->pos[2] += piece->vel[2];
        piece->rot[0] -= 0x180;
        if (i == 0) {
            piece->rot[1] += 0x200;
        } else {
            piece->rot[1] -= 0x200;
        }
        piece->vel[1] += -2.5f;
        piece->vel[0] *= 0.9f;
        piece->vel[1] *= 0.9f;
        piece->vel[2] *= 0.9f;
        if (piece->pos[1] <= AreaGrid2_GetFloorY(piece->pos)) {
            piece->alive = 0;
        }
        if (piece->node != NULL) {
            Vec_Copy(piece->node->pos, piece->pos);
            OT_AddByDepth(gDrawOt32, piece->node, 0x20, 0);
        }
        piece++;
    }
    if (alive == 0) {
        obj->state++;
        obj->visible = 0;
    }
}

/*
 * Draw callback of one ShotFlash piece: mesh 89 of the common model set at
 * the piece's rotation, added (Effect_DrawModelAdd).
 */
void ShotFlash_DrawPiece(DrawSlot *node)
{
    Model *mdl = gCommonModels[0];
    ShotFlashPiece *piece = (ShotFlashPiece *) ((StageObj *) node->owner)->buf + node->part;
    ModelMatSet *mesh;
    ModelMat *tex;
    f32 mtx[16];

    if (mdl != NULL && mdl->active != 0) {
        tex = mdl->mats;
        mesh = &mdl->matSets[89];
        Mtx_Identity(mtx);
        Mtx_RotateX(mtx, (6.2831855f * piece->rot[0]) / 65536.0f);
        Mtx_RotateY(mtx, (6.2831855f * piece->rot[1]) / 65536.0f);
        Mtx_TurnZ(mtx, (6.2831855f * piece->rot[2]) / 65536.0f);
        Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
        Effect_DrawModelAdd(mesh, mtx, tex);
    }
}

/*
 * End state of a ShotFlash: frees its parts' draw slots (the pieces of type
 * 4, the puffs of the others; the spark's own slot when the type has no
 * parts) and moves to the free state.
 */
void ShotFlash_End(StageObj *obj)
{
    ShotFlashPuff *puff;
    u8 *cursor = obj->buf;
    s16 count;
    s16 i;

    puff = (ShotFlashPuff *) (cursor + 0x40);
    obj->state++;
    count = ShotFlash_PartCounts[obj->type];
    if (count == 0) {
        DrawSlot_Free(obj->slot);
    } else {
        for (i = 0; i < count; i++) {
            if (obj->type == 4) {
                if (((ShotFlashPiece *) cursor)->node != NULL) {
                    DrawSlot_Free(((ShotFlashPiece *) cursor)->slot);
                }
                cursor += 0x28;
            } else {
                if (puff->node != NULL) {
                    DrawSlot_Free(puff->slot);
                }
                puff++;
            }
        }
    }
}

/* Free state of a ShotFlash: frees the object. */
void ShotFlash_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Plays a ShotFlash's sound at pos: only type 7 with color set 1 to 5 has one
 * (sound 0x26).
 */
void ShotFlash_PlaySound(StageObj *obj, f32 *pos)
{
    switch (obj->type) {
    case 7:
        switch (obj->var2) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, pos, 1, 0);
            break;
        }
        break;
    }
}

/*
 * Allocates a ShotFlash of type objType at size 1.0, with one block (the
 * spark, type 3, none). Returns it, or NULL when no object is free.
 */
StageObj *ShotFlash_Alloc(s32 objType)
{
    StageObj *obj;

    switch ((s16) objType) {
    default:
        obj = StageObj_Alloc(1);
        break;
    case 3:
        obj = StageObj_Alloc(0);
        break;
    }
    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_SHOT_FLASH;
        obj->update = ShotFlash_Update;
        obj->type = objType;
        obj->scale = 1.0f;
    }
    return obj;
}

/*
 * Creates a ShotFlash of type objType at bone bone of its owner (a pointer
 * passed as an integer), when the owner is in the current area.
 * Bowgun_FireShot makes types 0, 1 and 2 at the gun for the plain ammo.
 */
void ShotFlash_CreateOnBone(s32 owner, s16 bone, s32 objType)
{
    StageObj *obj;

    if (Obj_InCurrentArea((void *) owner) & 0xFF) {
        obj = ShotFlash_Alloc(objType);
        if (obj != NULL) {
            obj->angle = bone;
            obj->owner = (void *) owner;
        }
    }
}

/*
 * Creates a ShotFlash of type objType at pos with the color set colorSet
 * (var2). EmRangedAttack_Impact makes type 9, BowgunShot_GlowRun type 3.
 */
void ShotFlash_CreateAt(f32 *pos, s32 objType, u8 colorSet)
{
    StageObj *obj = ShotFlash_Alloc(objType);

    if (obj != NULL) {
        obj->var2 = colorSet;
        Vec_Copy(obj->pos, pos);
    }
}

/* As ShotFlash_CreateAt, with a size. */
void ShotFlash_CreateAtScaled(f32 *pos, s32 objType, u8 colorSet, f32 scale)
{
    StageObj *obj;

    if ((obj = ShotFlash_Alloc(objType)) != NULL) {
        obj->var2 = colorSet;
        Vec_Copy(obj->pos, pos);
        obj->scale = scale;
    }
}

/*
 * Creates a ShotFlash of type objType at an attack object's position (shot,
 * read by offset): the high bytes of its rotX and rotY go to var0 and var1,
 * half the length of its hit capsule (pos to pos2) is the scale.
 * BowgunShot_Fly makes the pieces (type 4) when a shot's life ends.
 */
void ShotFlash_CreateFromShot(u8 *shot, s32 objType, u8 colorSet)
{
    StageObj *obj;

    if ((obj = ShotFlash_Alloc(objType)) != NULL) {
        obj->var0 = *(s32 *) (shot + 0x24) >> 8;
        obj->var1 = *(s32 *) (shot + 0x28) >> 8;
        obj->var2 = colorSet;
        Vec_Copy(obj->pos, (f32 *) (shot + 0x30));
        obj->scale = 0.5f * Vec_Distance((f32 *) (shot + 0x30), (f32 *) (shot + 0x3C));
    }
}

/*
 * Creates a ShotFlash of type objType at bone bone of a player in the current
 * area, with the color set colorSet (var2) and the high byte of the player's
 * yaw (var1). Bowgun_FireShot makes types 5 and 6 for the colored ammo with
 * it.
 */
void ShotFlash_CreateOnBoneColored(Player *pl, s16 bone, s32 objType, u8 colorSet)
{
    StageObj *obj;

    if (Obj_InCurrentArea(pl) & 0xFF) {
        obj = ShotFlash_Alloc(objType);
        if (obj != NULL) {
            obj->var1 = (s8) ((s32) pl->rot[1] >> 8);
            obj->var2 = colorSet;
            obj->angle = bone;
            obj->owner = pl;
        }
    }
}

/*
 * Creates a ShotFlash of type objType at pos, without owner, with a pitch
 * (kept in timer) and a yaw (in angle) by which types 10 and 11 turn their
 * direction matrix. StageShot_Run makes types 10 and 11 when the mounted gun
 * fires.
 */
void ShotFlash_CreateAtAngles(f32 *pos, s32 objType, u16 pitch, u16 yaw)
{
    StageObj *obj;

    if ((obj = ShotFlash_Alloc(objType)) != NULL) {
        obj->timer = pitch;
        obj->angle = yaw;
        Vec_Copy(obj->pos, pos);
        obj->owner = NULL;
    }
}

/*
 * Creates the gunlance shell's ShotFlash (type 12) at bone 0xE of the player;
 * the shelling variant picks the puffs' start delay
 * (ShotFlash_GunlanceShellDelays).
 */
void ShotFlash_CreateGunlanceShell(Player *pl, s16 variant)
{
    ShotFlash_CreateOnBoneColored(pl, 0xE, 0xC, ShotFlash_GunlanceShellDelays[variant]);
}

/*
 * Update of an AlertMark: runs its set-up, run, end and free states by
 * obj->state.
 */
void AlertMark_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        AlertMark_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        AlertMark_Run(obj);
        break;
    case STAGEOBJ_END:
        AlertMark_End(obj);
        break;
    case STAGEOBJ_FREE:
        AlertMark_Free(obj);
        break;
    }
}

/*
 * Set-up state of an AlertMark: type 0 starts with no mark (var0 0) at frame
 * 15, type 1 with mark 3 at frame 0; takes one draw slot.
 */
void AlertMark_Setup(StageObj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    if (obj->type == 0) {
        obj->var0 = 0;
        obj->timer = 0xF;
    } else if (obj->type == 1) {
        obj->var0 = 3;
        obj->timer = 0;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->part = 0;
        obj->node->draw = AlertMark_Draw;
    } else {
        obj->node = NULL;
    }
}

/*
 * Run state of an AlertMark: ends when the owner is gone (NULL, inactive, or
 * a monster in its finish phase). For a monster owner (var2 nonzero) the mark
 * restarts each frame the monster's syncPulse is set (mark 2 for
 * syncPulse[1], else mark 1), moves along AlertMark_MoveKeys over 15 frames
 * and hides after 60. It sits at the owner's bone (obj->angle) plus the
 * kind's offset (AlertMark_OffsetsByEmKind) turned by the bone. A player
 * owner (var2 0) uses AlertMark_PlayerOffset turned by the camera, for 60
 * frames.
 */
void AlertMark_Run(StageObj *obj)
{
    Em *pl;
    Em *em;
    u8 *owner;
    f32 *ofs;
    u8 mark;
    s32 frame;
    f32 pos[3];
    f32 move[3];
    f32 boneMtx[16];

    if (obj->var2 == 0) {
        pl = (Em *) (owner = obj->owner);
    } else {
        em = (Em *) (owner = obj->owner);
    }
    if (owner == NULL || owner[0] == 0 || owner[4] == 2) {
        obj->state++;
        return;
    }
    move[0] = 0.0f;
    move[1] = 0.0f;
    move[2] = 0.0f;
    if (obj->var2 != 0) {
        if (*((s8 *) em + 0x7D8) == 0 && *((s8 *) em + 0x7D9) == 0) {
            mark = 0;
        } else if (*((s8 *) em + 0x7D9) != 0) {
            mark = 2;
        } else if (*((s8 *) em + 0x7D8) != 0) {
            mark = 1;
        } else {
            mark = 0;
        }
        if (mark == 0) {
            if (++obj->timer > 0x3C || obj->var0 == 0) {
                obj->timer = 0x3C;
                obj->var0 = 0;
                return;
            }
            frame = obj->timer;
            frame = (frame > 0xF) ? 0xF : frame;
            Key_LerpVec3(AlertMark_MoveKeys, move, frame);
        } else {
            obj->var0 = mark;
            obj->timer = 0;
        }
        Em_BonePos(em, obj->angle, pos);
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, obj->angle)));
        ofs = AlertMark_OffsetsByEmKind[em->type];
    } else {
        if (++obj->timer > 0x3C) {
            obj->state++;
            return;
        }
        Em_GetBonePos(pl, obj->angle, pos);
        Mtx_Copy(boneMtx, gCamMatrix);
        ofs = AlertMark_PlayerOffset;
    }
    move[0] += *ofs++;
    move[1] += *ofs++;
    move[2] += *ofs;
    Vec_RotateSelf(move, boneMtx);
    if (obj->node != NULL) {
        obj->node->pos[0] = pos[0] + move[0];
        obj->node->pos[1] = pos[1] + move[1];
        obj->node->pos[2] = pos[2] + move[2];
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
    }
}

/* End state of an AlertMark: frees its draw slot. */
void AlertMark_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* Free state of an AlertMark: frees the object. */
void AlertMark_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draw callback of an AlertMark, while its owner is in the current area: the
 * common model set's mesh of its mark (AlertMark_MarkMeshes by var0), facing
 * the camera, sized by AlertMark_ScaleKeys (type 0: its first 15 frames, 1.5
 * times) or AlertMark_FixedMarkScaleKeys.
 */
void AlertMark_Draw(DrawSlot *node)
{
    f32 mtx[16];
    f32 scale[3];
    s32 frame;
    StageObj *obj;
    ModelMat *tex;
    ModelMatSet *model;
    Model *mdl;

    mdl = gCommonModels[0];
    obj = node->owner;

    if (mdl != NULL && mdl->active != 0 && (Obj_InCurrentArea(obj->owner) & 0xFF)) {
        tex = mdl->mats;
        switch (obj->type) {
        case 0:
            frame = (obj->timer > 0xF) ? 0xF : obj->timer;
            Key_LerpVec3(AlertMark_ScaleKeys, scale, frame);
            scale[0] *= 1.5f;
            scale[1] *= 1.5f;
            scale[2] *= 1.5f;
            break;
        default:
            Key_LerpVec3(AlertMark_FixedMarkScaleKeys, scale, obj->timer);
            break;
        }
        Mtx_Scale(mtx, scale[0], scale[1], scale[2]);
        Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        model = &mdl->matSets[AlertMark_MarkMeshes[obj->var0]];
        if (model != NULL && model->id != -1) {
            Gs_SetState(0x60, 0);
            Gs_SetState(0x1A, (s32) mtx);
            Gs_SetState(0x67, -1);
            Gs_SetLightList(tex, model);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates an AlertMark of type objType for a monster at its bone bone (var2
 * 1: a monster owner). Ten monster families (EmRathalos_SpawnEffects and the
 * like) create type 0 at bone 0x22.
 */
void AlertMark_Create(void *owner, s16 bone, u8 objType)
{
    StageObj *obj = StageObj_Alloc(0);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_ALERT_MARK;
        obj->update = AlertMark_Update;
        obj->type = objType;
        obj->var2 = 1;
        obj->angle = bone;
        obj->owner = owner;
    }
}

/*
 * Update of a DustCloud: runs its set-up, run, end and free states by
 * obj->state.
 */
void DustCloud_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        DustCloud_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        DustCloud_Run(obj);
        break;
    case STAGEOBJ_END:
        DustCloud_End(obj);
        break;
    case STAGEOBJ_FREE:
        DustCloud_Free(obj);
        break;
    }
}

/*
 * Set-up state of a DustCloud. Bit 0x80 of the place code (var2), cleared
 * here, turns off the ground attack. With an owner it takes the owner's
 * heading (rotY); unless the owner is a player (var1 1), by type: 1, 9, 10
 * and 11 make the ground attack object (FatalisAttack for the Fatalis kinds,
 * RathalosAttack for the wyverns); some place codes turn types 2, 3 and 10
 * half a turn; type 7 gets a random size; 25 and 26 face a quarter turn from
 * the head bone's heading and keep the kind in var1. Each puff gets its start
 * time (DustCloud_StartTimes), size, look (flag), spin and a draw slot; types
 * 22 and 25 run their first frame at once.
 */
void DustCloud_Setup(StageObj *obj)
{
    s16 count;
    Em *em;
    s16 *delays;
    DustCloudPuff *puff;
    s16 makeAttack;
    s16 i;
    f32 headMtx[16];

    puff = (DustCloudPuff *) obj->buf;
    em = obj->owner;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    count = DustCloud_PartCounts[obj->type];
    delays = DustCloud_StartTimes[obj->type];
    if (obj->var2 & 0x80) {
        makeAttack = 0;
        obj->var2 ^= 0x80;
    } else {
        makeAttack = 1;
    }
    if (em != NULL) {
        obj->angle = em->rotY;
        if (obj->var1 != 1) {
            switch (obj->type) {
            case 0:
            case 1:
            case 9:
            case 11:
            case 14:
            case 18:
            case 19:
                switch (em->type) {
                case EM_KIND_FATALIS:
                case EM_KIND_FATALIS_B:
                case EM_KIND_FATALIS_C:
                    if (makeAttack) {
                        switch (obj->type) {
                        case 1:
                            FatalisAttack_CreateOnObj(obj, 0x15);
                            break;
                        }
                    }
                    break;
                case EM_KIND_RATHIAN:
                case EM_KIND_GOLD_RATHIAN:
                case EM_KIND_PINK_RATHIAN:
                case EM_KIND_YIAN_KUT_KU:
                case EM_KIND_BLUE_YIAN_KUT_KU:
                case EM_KIND_CEPHADROME:
                case EM_KIND_RATHALOS:
                case EM_KIND_SILVER_RATHALOS:
                case EM_KIND_AZURE_RATHALOS:
                case EM_KIND_DIABLOS:
                case EM_KIND_BLACK_DIABLOS:
                case EM_KIND_KHEZU:
                case EM_KIND_RED_KHEZU:
                case EM_KIND_GRAVIOS:
                case EM_KIND_BLACK_GRAVIOS:
                case EM_KIND_GYPCEROS:
                case EM_KIND_PURPLE_GYPCEROS:
                case EM_KIND_PLESIOTH:
                case EM_KIND_GREEN_PLESIOTH:
                case EM_KIND_BASARIOS:
                case EM_KIND_MONOBLOS:
                case EM_KIND_WHITE_MONOBLOS:
                case EM_KIND_CEPHALOS:
                case EM_KIND_YIAN_GARUGA:
                    if (makeAttack) {
                        switch (obj->type) {
                        case 1:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                RathalosAttack_CreateOnObj(obj, 0x21);
                            } else {
                                RathalosAttack_CreateOnObj(obj, 0x2C);
                            }
                            break;
                        case 9:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                RathalosAttack_CreateOnObj(obj, 0x27);
                            } else {
                                RathalosAttack_CreateOnObj(obj, 0x2E);
                            }
                            break;
                        case 11:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                RathalosAttack_CreateOnObj(obj, 0x26);
                            } else {
                                RathalosAttack_CreateOnObj(obj, 0x2D);
                            }
                            break;
                        }
                    }
                    break;
                }
                break;
            case 2:
            case 3:
                switch (obj->var2) {
                case 0x1B:
                case 0x1C:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x1E:
                case 0x2B:
                case 0x2F:
                    obj->angle += (s16) 0x8000;
                    break;
                }
                break;
            case 7:
                obj->scale *= 0.7f + 0.0002f * (Rand_Next(1) & 0x3FF);
                break;
            case 10:
                switch (em->type) {
                case EM_KIND_FATALIS:
                case EM_KIND_FATALIS_B:
                case EM_KIND_FATALIS_C:
                    if (makeAttack) {
                        FatalisAttack_CreateOnObj(obj, 0x15);
                    }
                    switch (obj->var2) {
                    case 0xB:
                    case 0x30:
                        obj->angle += (s16) 0x8000;
                        break;
                    }
                    break;
                case EM_KIND_RATHIAN:
                case EM_KIND_GOLD_RATHIAN:
                case EM_KIND_PINK_RATHIAN:
                case EM_KIND_YIAN_KUT_KU:
                case EM_KIND_BLUE_YIAN_KUT_KU:
                case EM_KIND_CEPHADROME:
                case EM_KIND_RATHALOS:
                case EM_KIND_SILVER_RATHALOS:
                case EM_KIND_AZURE_RATHALOS:
                case EM_KIND_DIABLOS:
                case EM_KIND_BLACK_DIABLOS:
                case EM_KIND_KHEZU:
                case EM_KIND_RED_KHEZU:
                case EM_KIND_GRAVIOS:
                case EM_KIND_BLACK_GRAVIOS:
                case EM_KIND_GYPCEROS:
                case EM_KIND_PURPLE_GYPCEROS:
                case EM_KIND_PLESIOTH:
                case EM_KIND_GREEN_PLESIOTH:
                case EM_KIND_BASARIOS:
                case EM_KIND_MONOBLOS:
                case EM_KIND_WHITE_MONOBLOS:
                case EM_KIND_CEPHALOS:
                case EM_KIND_YIAN_GARUGA:
                    if (makeAttack) {
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            RathalosAttack_CreateOnObj(obj, 0x20);
                        } else {
                            RathalosAttack_CreateOnObj(obj, 0x30);
                        }
                    }
                    break;
                }
                break;
            case 25:
                obj->var0 = Rand_Next(1) & 1;
                obj->var1 = em->type;
                if (obj->var1 == 2 || obj->var1 == 0x24 || obj->var1 == 0x47) {
                    Mtx_Copy((f32 *) headMtx, (f32 *) (Em_GetBone(em, 0x33)));
                } else {
                    Mtx_Copy((f32 *) headMtx, (f32 *) (Em_GetBone(em, 0x24)));
                }
                obj->angle = Mtx_Heading(headMtx) + 0x4000;
                break;
            case 26:
                obj->var1 = em->type;
                if (obj->var1 == 2 || obj->var1 == 0x24 || obj->var1 == 0x47) {
                    Mtx_Copy((f32 *) headMtx, (f32 *) (Em_GetBone(em, 0x33)));
                } else {
                    Mtx_Copy((f32 *) headMtx, (f32 *) (Em_GetBone(em, 0x24)));
                    if (obj->var1 == 0x16) {
                        if (em->stateGroup == 3 && em->state == 4 && em->motionId[0] == 0x456) {
                            obj->var0 = 1;
                        } else {
                            obj->var0 = 0;
                        }
                    }
                }
                obj->angle = Mtx_Heading(headMtx) + 0x4000;
                break;
            }
        }
    }
    for (i = 0; i < count; puff++, i++) {
        if (delays != NULL) {
            puff->time = delays[i];
        } else {
            puff->time = 0;
        }
        puff->size = obj->scale;
        puff->fixedAlpha = 0;
        switch (obj->type) {
        case 0:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            puff->rotv = (Rand_Next(1) & 0x1F) - 10;
            break;
        case 1:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            puff->rotv = 0.5f + (65536.0f * (0.15f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            puff->size = 10.0f * obj->scale;
            break;
        case 2:
        case 3:
            puff->rot = Rand_Next(1);
            if (i < 2) {
                puff->flag = 0;
                puff->rotv = 0.5f + (65536.0f * (0.4f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                puff->flag = 1;
                if (obj->var1 == 1) {
                    puff->size *= 0.4f;
                }
                puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 4:
            puff->rot = Rand_Next(1);
            if (i == 0) {
                puff->flag = 0;
                puff->rotv = 0.5f + (65536.0f * (0.3f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                puff->flag = 1;
                puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 5:
            puff->rot = Rand_Next(1);
            if (i < 5) {
                puff->flag = 0;
                puff->rotv = 0.5f + (65536.0f * (0.4f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                puff->flag = 1;
                puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 6:
            puff->flag = Rand_Next(1) & 1;
            puff->size = 0.8f + 0.00020000001f * (Rand_Next(1) & 0x3FF);
            puff->rot = Rand_Next(1);
            break;
        case 7:
        case 17:
            puff->flag = 0;
            puff->rot = Rand_Next(1);
            break;
        case 8:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            break;
        case 9:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            puff->size = 0.8f + 0.00020000001f * (Rand_Next(1) & 0x3FF);
            break;
        case 10:
        case 18:
            if (obj->type == 18) {
                puff->time = i * -3;
                puff->flag = 0;
            } else {
                puff->time = 0;
                puff->flag = i;
            }
            puff->rot = Rand_Next(1);
            if (i == 0) {
                puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                puff->rotv = 0.5f + (65536.0f * (0.03f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            puff->size = obj->scale * (0.8f + 0.00040000002f * (Rand_Next(1) & 0x3FF));
            break;
        case 11:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            puff->rotv = 0.5f + (65536.0f * (0.15f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            break;
        case 12:
            puff->rot = 0;
            puff->rotv = 0;
            break;
        case 13:
            puff->rot = Rand_Next(1);
            puff->rotv = 0;
            break;
        case 19:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            puff->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            break;
        case 14:
        case 15:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            break;
        case 16:
            puff->time = -i * 2;
            puff->flag = Rand_Next(1) & 1;
            puff->rot = 0;
            puff->size = 0.8f + 0.0003f * (Rand_Next(1) & 0x3FF);
            break;
        case 20:
            puff->flag = i;
            puff->rot = Rand_Next(1);
            if (i < 2) {
                puff->rotv = (Rand_Next(1) & 0x1FF) - 0x100;
            } else {
                puff->rotv = (Rand_Next(1) & 0xFF) - 0x80;
            }
            puff->size = obj->scale * (0.8f * DustCloud_PuffSizes[i]);
            break;
        case 21:
            puff->time = -i;
            puff->rot = Rand_Next(1);
            puff->rotv = (Rand_Next(1) & 0xFF) - 0x80;
            puff->flag = i;
            puff->fixedAlpha = 1;
            puff->size = obj->scale * (1.0f + 0.0005f * (Rand_Next(1) & 0x3FF));
            break;
        case 25:
            puff->flag = i;
            puff->time = -i;
            puff->fixedAlpha = 1;
            puff->alpha = 1.0f;
            puff->rot = Rand_Next(1);
            puff->rotv = 0.5f + (65536.0f * (0.2f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            puff->size = obj->scale * (0.8f + 0.00070000003f * (Rand_Next(1) & 0x3FF));
            Vec_Copy(puff->pos, &((Em *) obj->owner)->posX);
            break;
        case 26:
            puff->flag = Rand_Next(1) & 1;
            puff->fixedAlpha = 1;
            puff->alpha = 1.0f;
            puff->rot = Rand_Next(1);
            puff->rotv = (Rand_Next(1) & 0x3FF) - 0x200;
            puff->size = obj->scale * (1.0f + 0.00080000004f * (Rand_Next(1) & 0x3FF));
            break;
        case 28:
            puff->rot = Rand_Next(1);
            puff->size = 1.2f;
            puff->rotv = 0.5f + (65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200))) / 360.0f;
            break;
        case 29:
            puff->time = -i * 4;
            puff->rot = Rand_Next(1);
            puff->rotv = (Rand_Next(1) & 0x3FF) - 0x200;
            puff->flag = i;
            puff->fixedAlpha = 1;
            puff->size = obj->scale * (1.5f + 0.0005f * (Rand_Next(1) & 0x3FF));
            break;
        }
        puff->slot = DrawSlot_Alloc();
        if (puff->slot != -1) {
            puff->node = DrawSlot_Get(puff->slot);
            puff->node->owner = (void *) obj;
            puff->node->part = i;
            puff->node->draw = DustCloud_Draw;
        } else {
            puff->node = NULL;
        }
    }
    if (obj->type == 0x16 || obj->type == 0x19) {
        DustCloud_Run(obj);
    }
}

/*
 * Run state of a DustCloud: ends after DustCloud_Lifetimes frames, type 25
 * also when its owner goes (type 16 marks its puffs gone then); type 25
 * follows the owner's head bone unless its curve set is 3. Each puff, from
 * its start time and for its duration, takes its scale, offset (turned by the
 * yaw; spread on a circle in some types) and alpha from the type's
 * DustCloud_Curves and is queued for drawing. Type 16's puffs start at the
 * head with a random jitter and fall; their gone check compares the signed
 * flag with 0xFF, which never matches.
 */
void DustCloud_Run(StageObj *obj)
{
    DustCloudPuff *puff;
    s16 count;
    u16 dur;
    u16 total;
    u16 *durs;
    s16 step;
    s16 curveIdx;
    s16 curveSet;
    s16 i;
    void *curve;
    f32 ofs[3];
    f32 camOfs[3];
    f32 headMtx[16];

    puff = (DustCloudPuff *) obj->buf;
    count = DustCloud_PartCounts[obj->type];
    total = DustCloud_Lifetimes[obj->type];
    durs = DustCloud_PuffDurations[obj->type];
    curveIdx = DustCloud_CurveIdx[obj->type];
    step = DustCloud_CurvesPerPuff[obj->type];
    switch (obj->type) {
    case 7:
    case 12:
    case 13:
    case 26:
    case 28:
        dur = total;
        break;
    case 0:
        dur = 0x41;
        break;
    case 1:
        dur = 0x24;
        break;
    case 11:
        dur = 0x42;
        break;
    case 16:
        dur = 0x18;
        break;
    case 18:
        dur = 0x26;
        break;
    case 29:
        dur = 0x13;
        break;
    case 21:
        dur = 0x2D;
        break;
    case 25:
        curveSet = (u32) obj->var2 >> 1;
        curveIdx += curveSet * 4;
        break;
    }
    if (++obj->timer > total) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    if (obj->type == 0x10) {
        if (((Em *) obj->owner)->active == 0 || ((Em *) obj->owner)->taskPhase == 3) {
            for (i = 0; i < count; i++) {
                puff[i].flag = -1;
            }
        }
    } else if (obj->type == 0x19) {
        if (((Em *) obj->owner)->active == 0 || ((Em *) obj->owner)->taskPhase == 3) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (curveSet != 3) {
            Mtx_Copy((f32 *) headMtx, (f32 *) (Em_GetBone(obj->owner, 0x24)));
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 80.0f;
            Vec_RotateSelf(ofs, headMtx);
            obj->pos[0] = headMtx[12] + ofs[0];
            obj->pos[2] = headMtx[14] + ofs[2];
            obj->angle = Mtx_Heading(headMtx) + 0x4000;
        }
    }
    for (i = 0; i < count; i++, puff++) {
        switch (obj->type) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
        case 19:
        case 20:
        case 25:
            dur = durs[i];
            break;
        case 0:
        case 1:
        case 7:
        case 11:
        case 13:
        case 16:
        case 18:
        case 21:
        case 28:
        case 29:
            curveIdx = DustCloud_CurveIdx[obj->type];
            break;
        }
        if (++puff->time <= 0) {
            curveIdx += step;
        } else if (puff->time > (u16) dur) {
            curveIdx += step;
        } else {
            if (obj->type == 0x10 && puff->time == 1) {
                if (puff->flag != 0xFF) {
                    Em_BonePos(obj->owner, 0x24, puff->pos);
                    puff->pos[0] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    puff->pos[1] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    puff->pos[2] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                } else {
                    curveIdx += step;
                    continue;
                }
            }
        curve = DustCloud_Curves[curveIdx++];
        Key_LerpVec3(curve, puff->sc, puff->time);
        switch (obj->type) {
        case 0:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + i * 0x3333)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 1:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + i * 0x3333)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 4:
        case 5:
        case 9:
        case 10:
        case 12:
        case 18:
        case 19:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            if (obj->type == 1 || obj->type == 19) {
                ofs[0] *= obj->scale;
                ofs[1] *= obj->scale;
                ofs[2] *= obj->scale;
            } else if (obj->type == 18) {
                ofs[0] += DustCloud_SideOffsets[i];
                if (obj->var2 == 1) {
                    ofs[0] = -ofs[0];
                }
                ofs[2] += 100.0f;
            }
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 2:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] += 50.0f;
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 3:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] += 50.0f;
            ofs[0] = -ofs[0];
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 6:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            if (obj->var2 != 0) {
                ofs[0] = -ofs[0];
            }
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            break;
        case 7:
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = -1000.0f;
            Vec_RotateSelf(ofs, gCamMatrix);
            break;
        case 8:
        case 13:
        case 14:
        case 15:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            break;
        case 11:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + i * 0x3333)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 16:
            puff->pos[1] -= 10.25f;
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 0.0f;
            break;
        case 20:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            camOfs[0] = 0.0f;
            camOfs[1] = 0.0f;
            camOfs[2] = i * 10;
            Vec_RotateSelf(camOfs, gCamMatrix);
            ofs[0] += camOfs[0];
            ofs[1] += camOfs[1];
            ofs[2] += camOfs[2];
            puff->rot += (u16) puff->rotv;
            break;
        case 21:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + i * 0x2AAA)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 25:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            if ((obj->var2 & 1) == 1) {
                ofs[0] = -ofs[0];
            }
            ofs[1] += ((Em *) obj->owner)->posY - puff->pos[1];
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 26:
            ofs[0] = puff->time * (50.0f / dur);
            if (obj->var2 == 0) {
                ofs[0] = -ofs[0];
            }
            ofs[1] = 0.0f;
            ofs[2] = 0.0f;
            Vec_RotateY(ofs, (6.2831855f * obj->angle) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 28:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            if (obj->var2 == 1) {
                ofs[0] = -ofs[0];
            }
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + (i - 1) * 0x2AAB)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        case 29:
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpVec3(curve, ofs, puff->time);
            ofs[0] *= obj->scale;
            ofs[1] *= obj->scale;
            ofs[2] *= obj->scale;
            Vec_RotateY(ofs, (6.2831855f * (obj->angle + DustCloud_PuffSectors[i] * 0x2AAA)) / 65536.0f);
            puff->rot += (u16) puff->rotv;
            break;
        }
        if (puff->fixedAlpha == 0) {
            curve = DustCloud_Curves[curveIdx++];
            Key_LerpFloat(curve, &puff->alpha, puff->time);
        }
        if (puff->node != NULL) {
            if (obj->type == 0x10) {
                puff->node->pos[0] = puff->pos[0];
                puff->node->pos[1] = puff->pos[1];
                puff->node->pos[2] = puff->pos[2];
            } else {
                puff->node->pos[0] = obj->pos[0] + ofs[0];
                puff->node->pos[1] = obj->pos[1] + ofs[1];
                puff->node->pos[2] = obj->pos[2] + ofs[2];
            }
            OT_AddByDepth(gDrawOtSorted, puff->node, 0x40, 0);
        }
        }
    }
}

/*
 * End state of a DustCloud: frees each puff's draw slot (DustCloud_PartCounts
 * by type) and moves to the free state.
 */
void DustCloud_End(StageObj *obj)
{
    u8 *cursor = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = DustCloud_PartCounts[obj->type];
    for (i = 0; i < count; i++) {
        if (*(s32 *) (cursor + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) (cursor + 2));
        }
        cursor += 0x30;
    }
}

/* Free state of a DustCloud: frees the object. */
void DustCloud_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

s16 Effect_MapTexId(s16 id);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
s32 Model_DrawFade(void *mdl, f32 *m, u16 flags, void *a3, f32 alpha);
s16 EmSlot_FindKind(s16 k);
extern u8 DustCloud_FlatPuffColors[];
extern u8 DustCloud_RingColors[];
extern u8 DustCloud_RingColorsAlt[];
extern u8 DustCloud_BreathRathalosCore[];
extern u8 DustCloud_BreathRathalosTrail[];
extern u8 DustCloud_BreathGypcerosCore[];
extern u8 DustCloud_BreathGypcerosTrail[];
extern u8 DustCloud_BreathKutKuCore[];
extern u8 DustCloud_BreathKutKuTrail[];
extern u8 DustCloud_BreathKhezuCore[];
extern u8 DustCloud_BreathKhezuTrail[];
extern u8 DustCloud_BreathGraviosCore[];
extern u8 DustCloud_BreathGraviosTrail[];
extern u8 DustCloud_BreathBasariosCore[];
extern u8 DustCloud_BreathBasariosTrail[];
extern u8 DustCloud_BreathOtherCore[];
extern u8 DustCloud_BreathOtherTrail[];
extern u8 DustCloud_PuffRathalosFlame[];
extern u8 DustCloud_PuffRathalosSmoke[];
extern u8 DustCloud_PuffGypceros[];
extern u8 DustCloud_PuffKutKuFlame[];
extern u8 DustCloud_PuffKutKuSmoke[];
extern u8 DustCloud_PuffKhezu[];
extern u8 DustCloud_PuffPlesioth[];
extern u8 DustCloud_PuffCephadrome[];
extern u8 DustCloud_PuffGravios[];
extern u8 DustCloud_PuffBasarios[];
extern u8 DustCloud_PuffOther[];
extern u8 DustCloud_PuffFatalisFlame[];
extern u8 DustCloud_PuffFatalisSmoke[];
extern u8 *DustCloud_SectorColors[];

/*
 * Draw callback of one DustCloud puff, by type: mostly a puff texture of the
 * stage model set facing the camera, tinted with the area's effect color
 * (Effect_GetColor; type 12 white, type 20 a fixed brown) and faded by its
 * alpha. Types 7, 13 and 16 draw a model turned only about Y (7 and 13 not
 * facing the camera); 13, 21 and 29 take their color from key curves; 25 and
 * 26 pick per monster kind (var1) the monster's effect model
 * (gScene.partModels) or a puff texture, with that kind's color curve.
 */
void DustCloud_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    DustCloudPuff *puff = (DustCloudPuff *) (obj->buf + node->part * sizeof(DustCloudPuff));
    ModelMat *mats;
    u16 mode = 0;
    Model *set = gStageModelSet;
    Model *kindModel;
    void *mesh;
    u8 *curve;
    s32 index;
    s16 tex;
    f32 alphaScale;
    u32 color;
    u8 red, green, blue, areaAlpha;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 rot[3];
    f32 scale[3];

    if (set != NULL && set->active) {
        mats = set->mats;
        Effect_GetColor(gScene.area, &red, &green, &blue, &areaAlpha);
        alphaScale = areaAlpha / 255.0f;
        switch (obj->type) {
        case 0:
        case 1:
        case 9:
        case 11:
        case 12:
        case 14:
        case 19:
        case 20:
        case 28:
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if (obj->type == 0xE) {
                if ((index = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * alphaScale * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            } else {
                if ((index = Effect_MapTexId(0x4A)) != -1) {
                    mesh = &set->matSets[index];
                    if (obj->type == 0xC) {
                        red = 0xFF;
                        green = 0xFF;
                        blue = 0xFF;
                        alphaScale = 1.0f;
                    } else if (obj->type == 0x14) {
                        red = 0x7F;
                        green = 0x6D;
                        blue = 0x54;
                        alphaScale = 1.0f;
                    }
                    Effect_DrawModel(mesh, mtx, ((u8) (255.0f * alphaScale * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            }
            break;
        case 2:
        case 3:
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            if (obj->type == 3) {
                Mtx_RotateY(mtx, 3.1415927f);
            }
            Mtx_Concat3x3(mtx, gCamMatrix);
            if (puff->flag == 0) {
                if ((index = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * alphaScale * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            } else {
                if ((index = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            }
            break;
        case 4:
        case 5:
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(puff->sc, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if (puff->flag == 0) {
                if ((index = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * alphaScale * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            } else {
                if ((index = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * puff->alpha) << 24) | 0x555555, 0, mats);
                }
            }
            break;
        case 6:
        case 8:
        case 15:
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if ((index = Effect_MapTexId(0x55)) != -1) {
                Model_DrawFade(&set->matSets[index], mtx, 0, mats, puff->alpha);
            }
            break;
        case 7:
            Gs_SetState(0x6C, 0);
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[1] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, obj->pos, 2, mtx);
            if ((index = Effect_MapTexId(0x5D)) != -1) {
                mesh = &set->matSets[index];
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                Model_DrawFade(mesh, mtx, 0, mats, puff->alpha);
            }
            break;
        case 10:
        case 18:
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if (puff->flag == 0) {
                if ((index = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[index], mtx, ((u8) (255.0f * alphaScale * puff->alpha) << 24) | (red << 16) | (green << 8) | blue, 0, mats);
                }
            } else {
                if ((index = Effect_MapTexId(0x23)) != -1) {
                    Model_DrawFade(&set->matSets[index], mtx, 0, mats, puff->alpha);
                }
            }
            break;
        case 13:
            Gs_SetState(0x6C, 0);
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[1] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, mtx);
            if ((index = Effect_MapTexId(0x2A)) != -1) {
                mesh = &set->matSets[index];
                Color_EvalKeys((s32 *) DustCloud_FlatPuffColors, puff->time, &color);
                Effect_DrawModel(mesh, mtx, color, 2, mats);
            }
            break;
        case 16:
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            if (puff->flag != 0) {
                rot[1] = 0.0f;
            } else {
                rot[1] = 3.1415927f;
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if ((index = Effect_MapTexId(0x24)) != -1) {
                Model_DrawFade(&set->matSets[index], mtx, 0, mats, puff->alpha);
            }
            break;
        case 21:
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if ((index = Effect_MapTexId(0x4A)) != -1) {
                mesh = &set->matSets[index];
                switch (obj->var2) {
                default:
                    curve = DustCloud_RingColors;
                    break;
                case 1:
                    curve = DustCloud_RingColorsAlt;
                    break;
                }
                Color_EvalKeys((s32 *) curve, puff->time, &color);
                Effect_DrawModel(mesh, mtx, color, 0, mats);
            }
            break;
        case 25:
            if ((index = EmSlot_FindKind(obj->var1)) == -1) {
                return;
            }
            kindModel = gScene.partModels[index];
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            switch (obj->var1) {
            case 1:
            case 11:
                if (kindModel == NULL) {
                    return;
                }
                if (!kindModel->active) {
                    return;
                }
                if (puff->flag == 0) {
                    curve = DustCloud_BreathRathalosCore;
                    mesh = &kindModel->matSets[2];
                    mats = kindModel->mats;
                    Mtx_Translation(uvMtx, 0.25f * obj->var0, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32) uvMtx);
                    mode |= 2;
                } else {
                    if ((index = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[index];
                    mats = set->mats;
                    curve = DustCloud_BreathRathalosTrail;
                }
                break;
            case 20:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                mats = set->mats;
                if (puff->flag == 0) {
                    curve = DustCloud_BreathGypcerosCore;
                } else {
                    curve = DustCloud_BreathGypcerosTrail;
                }
                break;
            case 6:
                if (kindModel == NULL) {
                    return;
                }
                if (!kindModel->active) {
                    return;
                }
                if (puff->flag == 0) {
                    mats = kindModel->mats;
                    curve = DustCloud_BreathKutKuCore;
                    mesh = &kindModel->matSets[2];
                } else {
                    if ((index = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[index];
                    mats = set->mats;
                    curve = DustCloud_BreathKutKuTrail;
                }
                break;
            case 15:
                if ((index = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                mats = set->mats;
                mode |= 2;
                if (puff->flag == 0) {
                    curve = DustCloud_BreathKhezuCore;
                } else {
                    curve = DustCloud_BreathKhezuTrail;
                }
                break;
            case 17:
                if ((index = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                mats = set->mats;
                mode |= 2;
                if (puff->flag == 0) {
                    curve = DustCloud_BreathGraviosCore;
                } else {
                    curve = DustCloud_BreathGraviosTrail;
                }
                break;
            case 22:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                mats = set->mats;
                if (puff->flag == 0) {
                    curve = DustCloud_BreathBasariosCore;
                } else {
                    curve = DustCloud_BreathBasariosTrail;
                }
                break;
            case 14:
            case 26:
            default:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                mats = set->mats;
                if (puff->flag == 0) {
                    curve = DustCloud_BreathOtherCore;
                } else {
                    curve = DustCloud_BreathOtherTrail;
                }
                break;
            }
            Color_EvalKeys((s32 *) curve, puff->time, &color);
            Effect_DrawModel(mesh, mtx, color, mode, mats);
            break;
        case 26:
            if ((index = EmSlot_FindKind(obj->var1)) == -1) {
                return;
            }
            kindModel = gScene.partModels[index];
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            switch (obj->var1) {
            case 1:
            case 0x2A:
            case 0x25:
            case 0xB:
            case 0x29:
            case 0x31:
                if (kindModel == NULL) {
                    return;
                }
                if (!kindModel->active) {
                    return;
                }
                if (puff->flag == 0) {
                    mode |= 2;
                    curve = DustCloud_PuffRathalosFlame;
                    mats = kindModel->mats;
                    mesh = &kindModel->matSets[2];
                    Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
                } else {
                    if ((index = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[index];
                    curve = DustCloud_PuffRathalosSmoke;
                    mats = set->mats;
                }
                break;
            case 0x14:
            case 0x27:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffGypceros;
                mats = set->mats;
                break;
            case 6:
            case 0x26:
                if (kindModel == NULL) {
                    return;
                }
                if (!kindModel->active) {
                    return;
                }
                if (puff->flag == 0) {
                    curve = DustCloud_PuffKutKuFlame;
                    mats = kindModel->mats;
                    mesh = &kindModel->matSets[2];
                } else {
                    mats = set->mats;
                    if ((index = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[index];
                    curve = DustCloud_PuffKutKuSmoke;
                }
                break;
            case 0xF:
            case 0x2D:
                mode |= 2;
                if ((index = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffKhezu;
                mats = set->mats;
                break;
            case 0x15:
            case 0x2E:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffPlesioth;
                mats = set->mats;
                break;
            case 8:
            case 0x22:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffCephadrome;
                mats = set->mats;
                break;
            case 0x11:
            case 0x2F:
                mode |= 2;
                if ((index = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffGravios;
                mats = set->mats;
                break;
            case 0x16:
                mats = set->mats;
                if (obj->var0 != 0) {
                    tex = 0x84;
                    mode |= 2;
                    curve = DustCloud_PuffGravios;
                } else {
                    tex = 0x4A;
                    curve = DustCloud_PuffBasarios;
                }
                if ((index = Effect_MapTexId(tex)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                break;
            case 0xE:
            case 0x2B:
            case 0x1A:
            case 0x2C:
            case 0x47:
            default:
                if ((index = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[index];
                curve = DustCloud_PuffOther;
                mats = set->mats;
                break;
            case 2:
            case 0x24:
                if (kindModel == NULL) {
                    return;
                }
                if (!kindModel->active) {
                    return;
                }
                if (puff->flag == 0) {
                    mode |= 2;
                    curve = DustCloud_PuffFatalisFlame;
                    mats = kindModel->mats;
                    mesh = &kindModel->matSets[2];
                    Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
                } else {
                    mats = set->mats;
                    if ((index = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[index];
                    curve = DustCloud_PuffFatalisSmoke;
                }
                break;
            }
            Gs_SetState(0x19, (s32) uvMtx);
            Color_EvalKeys((s32 *) curve, puff->time, &color);
            Effect_DrawModel(mesh, mtx, color, mode, mats);
            break;
        case 29:
            scale[0] = puff->size * puff->sc[0];
            scale[1] = puff->size * puff->sc[1];
            scale[2] = puff->size * puff->sc[2];
            rot[2] = (6.2831855f * puff->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            if ((index = Effect_MapTexId(0x4A)) != -1) {
                mesh = &set->matSets[index];
                Color_EvalKeys((s32 *) DustCloud_SectorColors[obj->var2], puff->time, &color);
                Effect_DrawModel(mesh, mtx, color, 0, mats);
            }
            break;
        default:
            break;
        }
    end:
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Looks for a special surface under pos for a DustCloud of type objType:
 * returns 0 at once when its DustCloud_SurfaceFx entry is 0. On a surface no
 * more than 18 below pos it puts pos 5 above it and returns the entry;
 * otherwise pos goes to em's floorY, and in the fortress areas 5 above it
 * (plus Area_GroundOffset) with the entry returned; else 0. em is the monster
 * or player.
 */
s16 DustCloud_FindGround(Em *em, f32 *pos, s16 objType)
{
    s16 *surfaceFx = &DustCloud_SurfaceFx[objType];
    s16 areaKind;
    f32 y;

    if (*surfaceFx == 0) {
        return 0;
    }
    if (AreaGrid2_GetFloorHeight(pos, &y)) {
        if (y < pos[1] - 18.0f) {
            pos[1] = em->floorY;
        } else {
            pos[1] = 5.0f + y;
            return *surfaceFx;
        }
    } else {
        pos[1] = em->floorY;
    }
    areaKind = gAreaKinds[gScene.area];
    if (areaKind == 10 || areaKind == 14) {
        pos[1] = 5.0f + em->floorY;
        pos[1] += Area_GroundOffset(gScene.area);
        return *surfaceFx;
    }
    return 0;
}

/*
 * Puts in pos where a DustCloud of type objType with place code place goes
 * for a monster: by type and kind, under a bone on the floor, at the monster,
 * at an offset of its head (types 25, 26) or ahead of bone 0x23 (types 6 to
 * 8, 15). For types 1, 9, 10 and 11 of the wyvern kinds it picks a ground
 * attack kind (none with bit 0x80 of place), made here with
 * RathalosAttack_CreateAt when the place has a special surface. Returns
 * DustCloud_FindGround's result.
 */
s16 DustCloud_PlaceAtEm(f32 *pos, Em *em, s16 objType, s16 place)
{
    s16 surfaceFx;
    u8 yset;
    s16 makeAttack;
    s16 attackKind;
    f32 ofs[3];
    f32 boneMtx[16];

    yset = 0;
    attackKind = 0;
    if (place & 0x80) {
        makeAttack = 0;
        place ^= 0x80;
    } else {
        makeAttack = 1;
    }
    switch (objType) {
    case 0:
    case 1:
    case 9:
    case 11:
    case 14:
    case 18:
    case 19:
        switch (em->type) {
        case EM_KIND_FATALIS:
        case EM_KIND_FATALIS_B:
        case EM_KIND_FATALIS_C:
            switch (place) {
            case 0:
                Em_BonePos(em, 0x1F, pos);
                break;
            default:
                Em_BonePos(em, 0x13, pos);
                break;
            }
            pos[1] = em->floorY;
            break;
        case EM_KIND_BLANGONGA:
        case EM_KIND_CONGALALA:
        case EM_KIND_RAJANG:
        case EM_KIND_BLANGO:
        case EM_KIND_CONGA:
            switch (place) {
            case 19:
                Em_BonePos(em, 0x10, pos);
                break;
            case 17:
                Em_BonePos(em, 0xE, pos);
                break;
            case 20:
                Em_BonePos(em, 4, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case EM_KIND_DAIMYO_HERMITAUR:
        case EM_KIND_HERMITAUR:
        case EM_KIND_CEANATAUR:
        case EM_KIND_SHOGUN_CEANATAUR:
            switch (place) {
            case 21:
                Em_BonePos(em, 0xC, pos);
                break;
            case 24:
                Em_BonePos(em, 0xB, pos);
                break;
            case 37:
                Em_BonePos(em, 0xD, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            switch (place) {
            case 25:
                Em_BonePos(em, 0x1F, pos);
                break;
            case 29:
                Em_BonePos(em, 0x1F, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case EM_KIND_SHEN_GAOREN:
            switch (place) {
            case 38:
                Em_BonePos(em, 0xA, pos);
                break;
            case 39:
                Em_BonePos(em, 6, pos);
                break;
            case 40:
                Em_BonePos(em, 2, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        default:
            switch (place) {
            default:
                pos[0] = em->posX;
                pos[1] = em->floorY;
                pos[2] = em->posZ;
                break;
            case 9:
                Em_BonePos(em, 4, pos);
                pos[1] = em->floorY;
                break;
            }
            if (makeAttack) {
                switch (em->type) {
                case 2:
                case 0x24:
                case 0x47:
                    if (makeAttack) { break; }
                    break;
                case 0x1:
                case 0x2A:
                case 0x25:
                case 0x6:
                case 0x26:
                case 0x8:
                case 0xB:
                case 0x29:
                case 0x31:
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
                    switch (objType) {
                    case 1:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            attackKind = 0x21;
                        } else {
                            attackKind = 0x2C;
                        }
                        break;
                    case 9:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            attackKind = 0x27;
                        } else {
                            attackKind = 0x2E;
                        }
                        break;
                    case 11:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            attackKind = 0x26;
                        } else {
                            attackKind = 0x2D;
                        }
                        break;
                    }
                    break;
                }
            }
            break;
        }
        break;
    case 2:
    case 3:
        switch (place) {
        case 4:
            Em_BonePos(em, 0xA, pos);
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 20.0f;
            Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
            pos[0] += ofs[0];
            pos[2] += ofs[2];
            break;
        case 5:
            Vec_Copy(pos, &em->posX);
            if (objType == 2) {
                ofs[0] = -85.0f;
            } else {
                ofs[0] = 85.0f;
            }
            ofs[1] = 0.0f;
            ofs[2] = 0.0f;
            Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
            yset = 1;
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            break;
        case 6:
            Em_BonePos(em, 2, pos);
            break;
        case 8:
            Em_BonePos(em, 0x1C, pos);
            break;
        case 11:
            Em_BonePos(em, 0x1E, pos);
            break;
        case 48:
            Em_BonePos(em, 0x19, pos);
            break;
        case 12:
            Em_BonePos(em, 2, pos);
            break;
        case 13:
            Em_BonePos(em, 1, pos);
            break;
        case 16:
            Em_BonePos(em, 0x17, pos);
            break;
        case 18:
        case 34:
            Em_BonePos(em, 9, pos);
            break;
        case 15:
            Em_BonePos(em, 0x1B, pos);
            break;
        case 17:
        case 33:
            Em_BonePos(em, 0xE, pos);
            break;
        case 23:
        case 36:
            Em_BonePos(em, 6, pos);
            break;
        case 22:
        case 35:
            Em_BonePos(em, 0xA, pos);
            break;
        case 25:
            Em_BonePos(em, 8, pos);
            break;
        case 26:
            Em_BonePos(em, 0xD, pos);
            break;
        case 28:
            Em_BonePos(em, 0x1A, pos);
            break;
        case 29:
        case 30:
            Em_BonePos(em, 0x1F, pos);
            break;
        case 31:
            Em_BonePos(em, 0x27, pos);
            break;
        case 32:
            Em_BonePos(em, 0x12, pos);
            break;
        case 38:
            Em_BonePos(em, 0xA, pos);
            break;
        case 39:
            Em_BonePos(em, 6, pos);
            break;
        case 41:
            Em_BonePos(em, 0xD, pos);
            break;
        case 42:
        case 47:
            Em_BonePos(em, 0x1F, pos);
            break;
        case 43:
            Em_BonePos(em, 0x29, pos);
            break;
        case 44:
            Em_BonePos(em, 0x17, pos);
            break;
        case 45:
            Em_BonePos(em, 6, pos);
            break;
        case 46:
            Em_BonePos(em, 9, pos);
            break;
        default:
            if (objType == 2) {
                Em_BonePos(em, 0x1A, pos);
                if (place == 2) {
                    ofs[0] = 0.0f;
                    ofs[1] = 0.0f;
                    ofs[2] = -50.0f;
                    Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
                    pos[0] += ofs[0];
                    pos[2] += ofs[2];
                }
            } else {
                Em_BonePos(em, 0x14, pos);
            }
            break;
        }
        if (yset == 0) {
            pos[1] = em->floorY;
        }
        break;
    case 4:
        if (place == 0) {
            Em_BonePos(em, 0x15, pos);
        } else {
            Em_BonePos(em, 0x1B, pos);
        }
        pos[1] = em->floorY;
        break;
    case 5:
        Em_BonePos(em, 0x1B, pos);
        pos[1] = em->floorY;
        break;
    case 6:
    case 15:
        Em_BonePos(em, 0x23, pos);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 60.0f;
        Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += ofs[0];
        pos[1] += ofs[1];
        pos[2] += ofs[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 8:
        Em_BonePos(em, 0x23, pos);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 30.0f;
        Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += ofs[0];
        pos[1] += ofs[1];
        pos[2] += ofs[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 7:
        Em_BonePos(em, 0x23, pos);
        ofs[0] = 0.1f * ((Rand_Next(1) & 0x3FF) - 0x200);
        ofs[1] = 0.0f;
        ofs[2] = 80.0f + 0.1f * ((Rand_Next(1) & 0x3FF) - 0x200);
        Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += ofs[0];
        pos[2] += ofs[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        pos[1] += -5.0f;
        break;
    case 10:
        if (place != 0x30) {
            if (place != 0xB) {
                switch (em->type) {
                case EM_KIND_FATALIS:
                case EM_KIND_FATALIS_B:
                case EM_KIND_FATALIS_C:
                    Em_BonePos(em, 1, pos);
                    pos[1] = AreaGrid2_GetFloorY(pos);
                    break;
                case EM_KIND_RATHIAN:
                case EM_KIND_GOLD_RATHIAN:
                case EM_KIND_PINK_RATHIAN:
                case EM_KIND_YIAN_KUT_KU:
                case EM_KIND_BLUE_YIAN_KUT_KU:
                case EM_KIND_CEPHADROME:
                case EM_KIND_RATHALOS:
                case EM_KIND_SILVER_RATHALOS:
                case EM_KIND_AZURE_RATHALOS:
                case EM_KIND_DIABLOS:
                case EM_KIND_BLACK_DIABLOS:
                case EM_KIND_KHEZU:
                case EM_KIND_RED_KHEZU:
                case EM_KIND_GRAVIOS:
                case EM_KIND_BLACK_GRAVIOS:
                case EM_KIND_GYPCEROS:
                case EM_KIND_PURPLE_GYPCEROS:
                case EM_KIND_PLESIOTH:
                case EM_KIND_GREEN_PLESIOTH:
                case EM_KIND_BASARIOS:
                case EM_KIND_MONOBLOS:
                case EM_KIND_WHITE_MONOBLOS:
                case EM_KIND_CEPHALOS:
                case EM_KIND_YIAN_GARUGA:
                    Em_BonePos(em, 0x29, pos);
                    pos[1] = AreaGrid2_GetFloorY(pos);
                    if (makeAttack) {
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            attackKind = 0x20;
                        } else {
                            attackKind = 0x30;
                        }
                    }
                    break;
                }
            } else {
                Em_BonePos(em, 0x1E, pos);
                pos[1] = AreaGrid2_GetFloorY(pos);
            }
        } else {
            Em_BonePos(em, 0x19, pos);
            pos[1] = AreaGrid2_GetFloorY(pos);
        }
        break;
    case 13:
    case 21:
    case 29:
        pos[0] = em->posX;
        pos[1] = em->floorY;
        pos[2] = em->posZ;
        break;
    case 25:
        if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, 0x33)));
        } else {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, 0x24)));
        }
        Mtx_GetTrans(pos, boneMtx);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 80.0f * em->scaleZ;
        Vec_RotateSelf(ofs, boneMtx);
        pos[0] += ofs[0];
        pos[1] += ofs[1];
        pos[2] += ofs[2];
        break;
    case 26:
        if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, 0x33)));
            ofs[0] = 0.0f;
            ofs[1] = -23.7f;
            ofs[2] = 151.9f;
        } else {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, 0x24)));
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 50.0f * em->scaleZ;
        }
        Mtx_GetTrans(pos, boneMtx);
        Vec_RotateSelf(ofs, boneMtx);
        pos[0] += ofs[0];
        pos[1] += ofs[1];
        pos[2] += ofs[2];
        break;
    case 28:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, 2)));
        if (place == 0) {
            ofs[0] = -65.0f;
        } else {
            ofs[0] = 65.0f;
        }
        ofs[1] = 0.0f;
        ofs[2] = 0.0f;
        Vec_RotateY(ofs, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] = boneMtx[12] + ofs[0];
        pos[1] = em->floorY;
        pos[2] = boneMtx[14] + ofs[2];
        break;
    }
    if ((surfaceFx = DustCloud_FindGround(em, pos, objType)) != 0 && attackKind != 0) {
        RathalosAttack_CreateAt(em, pos, attackKind);
    }
    return surfaceFx;
}

/*
 * Creates a DustCloud of type objType for a monster at the place
 * DustCloud_PlaceAtEm finds, when a player is in its area; on a special
 * surface DustCloud_SpawnSurfaceFx shows its effects instead and no object is
 * made. var2 keeps the place code, var1 0 marks a monster owner.
 */
void DustCloud_Create(Em *em, s16 objType, s16 place, f32 scale)
{
    StageObj *obj;
    f32 pos[3];

    if ((Player_InCurrentArea(em) & 0xFF) && DustCloud_SpawnSurfaceFx(em, pos, DustCloud_PlaceAtEm(pos, em, objType, place), scale) == 0) {
        if ((obj = StageObj_Alloc(1)) != NULL) {
            obj->kind = STAGEOBJ_KIND_DUST_CLOUD;
            obj->update = DustCloud_Update;
            obj->type = objType;
            obj->owner = em;
            obj->scale = scale;
            Vec_Copy(obj->pos, pos);
            obj->var2 = place;
            obj->var1 = 0;
        }
    }
}

/*
 * Creates a DustCloud of type objType at pos with a yaw, without owner (var1
 * 2). PitfallTrap_Run makes types 12 and 20.
 */
void DustCloud_CreateAtYaw(f32 *pos, s32 objType, u16 yaw, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = STAGEOBJ_KIND_DUST_CLOUD;
        obj->update = DustCloud_Update;
        obj->type = objType;
        obj->owner = NULL;
        obj->scale = scale;
        obj->var2 = 0;
        obj->angle = yaw;
        Vec_Copy(obj->pos, pos);
        obj->var1 = 2;
    }
}

/*
 * Creates a DustCloud for a player in the current area: type 2 under bone 8
 * or type 3 under bone 5, on the floor; place 2 or 3 moves it 50 forward or
 * back. On a special surface the surface effects show instead. var1 1 marks a
 * player owner. The player's motion effects call it for footsteps.
 */
void DustCloud_CreateOnPlayer(Player *pl, s16 objType, s16 place, f32 scale)
{
    StageObj *obj;
    f32 pos[3];
    f32 ofs[3];

    if (Obj_InCurrentArea(pl) & 0xFF) {
        switch (objType) {
        case 2:
            Em_GetBonePos(pl, 8, pos);
            pos[1] = pl->floorY;
            if (place == 2) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = 50.0f;
                Vec_RotateY(ofs, (6.2831855f * pl->rot[1]) / 65536.0f);
                pos[0] += ofs[0];
                pos[1] += ofs[1];
                pos[2] += ofs[2];
            } else if (place == 3) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = -50.0f;
                Vec_RotateY(ofs, (6.2831855f * pl->rot[1]) / 65536.0f);
                pos[0] += ofs[0];
                pos[1] += ofs[1];
                pos[2] += ofs[2];
            }
            break;
        case 3:
            Em_GetBonePos(pl, 5, pos);
            pos[1] = pl->floorY;
            if (place == 2) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = 50.0f;
                Vec_RotateY(ofs, (6.2831855f * pl->rot[1]) / 65536.0f);
                pos[0] += ofs[0];
                pos[1] += ofs[1];
                pos[2] += ofs[2];
            } else if (place == 3) {
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = -50.0f;
                Vec_RotateY(ofs, (6.2831855f * pl->rot[1]) / 65536.0f);
                pos[0] += ofs[0];
                pos[1] += ofs[1];
                pos[2] += ofs[2];
            }
            break;
        }
        if (DustCloud_SpawnSurfaceFx((Em *) pl, pos, DustCloud_FindGround((Em *) pl, pos, objType), scale) == 0) {
            if ((obj = StageObj_Alloc(1)) != NULL) {
                obj->kind = STAGEOBJ_KIND_DUST_CLOUD;
                obj->update = DustCloud_Update;
                obj->type = objType;
                obj->owner = pl;
                Vec_Copy(obj->pos, pos);
                obj->scale = scale;
                obj->var2 = place;
                obj->var1 = 1;
            }
        }
    }
}

/*
 * Creates a DustCloud of type objType at pos, without owner (var1 2), with
 * the variant variant (var2) and a size.
 */
void DustCloud_CreateAt(f32 *pos, u8 objType, u8 variant, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = STAGEOBJ_KIND_DUST_CLOUD;
        obj->update = DustCloud_Update;
        obj->type = objType;
        obj->owner = NULL;
        obj->scale = scale;
        Vec_Copy(obj->pos, pos);
        obj->var2 = variant;
        obj->var1 = 2;
    }
}

/*
 * Shows the surface effects at pos in place of a DustCloud. mode is
 * DustCloud_FindGround's result: 0 shows nothing and returns 0; otherwise a
 * set of Effect08 particles sized by scale (mode 2 and 3 their own sets) and
 * sound 0x76, or 0x70 when the last size is at most 4; returns 1. em is
 * unused.
 */
s8 DustCloud_SpawnSurfaceFx(Em *em, f32 *pos, s32 mode, f32 scale)
{
    f32 size;

    switch ((s16) mode) {
    case 0:
        return 0;
    default:
    case 1:
        size = 5.0f * scale;
        Effect08_Spawn(pos, 4, 0, size);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, size);
        break;
    case 2:
        Effect08_Spawn(pos, 4, 0, 5.0f * scale);
        pos[1] += 5.0f;
        size = 4.5f * scale;
        Effect08_Spawn(pos, 1, 0, size);
        break;
    case 3:
        size = 10.0f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 2, 0, size);
        break;
    }
    if (!(size <= 4.0f)) {
        Sound_PlaySeAtPosChecked(1, 0x76, 0, pos, 1, 0);
    } else {
        Sound_PlaySeAtPosChecked(1, 0x70, 0, pos, 1, 0);
    }
    return 1;
}

/*
 * Creates a FallingDebris of type objType for a monster in the current area.
 */
void FallingDebris_Create(Em *em, u8 objType)
{
    StageObj *obj;

    if (Obj_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_FALLING_DEBRIS;
            obj->update = FallingDebris_Update;
            obj->draw = NULL;
            obj->owner = em;
            obj->ownerIndex = em->index;
            obj->type = objType;
        }
    }
}

/*
 * Update of a FallingDebris: runs its set-up, run, end and free states by
 * obj->state.
 */
void FallingDebris_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        FallingDebris_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        FallingDebris_Run(obj);
        break;
    case STAGEOBJ_END:
        FallingDebris_End(obj);
        break;
    case STAGEOBJ_FREE:
        FallingDebris_Free(obj);
        break;
    }
}

/*
 * Set-up state of a FallingDebris: the start point is bone 0xA (type 0),
 * Gravios' bone 0x24 or an offset of Kushala Daora's bone 0x25 (type 1; other
 * kinds free the object), or an offset of bone 0x27 (type 2). Three pieces
 * start around it with a random size, wait, rotation and a slow push along
 * the bone's direction, each with a draw slot; obj->pos[1] becomes the floor
 * height. Frees the object when no piece got a draw slot.
 */
void FallingDebris_Setup(StageObj *obj)
{
    Em *em = obj->owner;
    FallingDebrisPiece *piece = (FallingDebrisPiece *) obj->buf;
    s16 failed;
    s16 i;
    f32 sizeFactor;
    f32 boneMtx[16];
    f32 ofs[3];

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    default:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, 0xA)));
        sizeFactor = 1.0f;
        Mtx_GetTrans(obj->pos, boneMtx);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, 0x24)));
            sizeFactor = 1.5f;
            Mtx_GetTrans(obj->pos, boneMtx);
            break;
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, 0x25)));
            ofs[0] = 0.0f;
            sizeFactor = 1.5f;
            ofs[1] = -45.0f;
            ofs[2] = 165.0f;
            Vec_RotateSelf(ofs, boneMtx);
            Mtx_GetTrans(obj->pos, boneMtx);
            obj->pos[0] += ofs[0];
            obj->pos[1] += ofs[1];
            obj->pos[2] += ofs[2];
            break;
        default:
            StageObj_Free(obj);
            return;
        }
        break;
    case 2:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, 0x27)));
        sizeFactor = 1.5f;
        ofs[0] = 0.0f;
        ofs[1] = 30.0f;
        ofs[2] = 100.0f;
        Mtx_GetTrans(obj->pos, boneMtx);
        Vec_RotateSelf(ofs, boneMtx);
        Vec_Add(obj->pos, obj->pos, ofs);
        Mtx_TurnY(boneMtx, 3.1415927f);
        break;
    }
    failed = 1;
    for (i = 0; i < 3; i++, piece++) {
        piece->phase = 0;
        piece->visible = 0;
        piece->scale = sizeFactor * (0.3f + 0.01f * (s32) (Rand_Next(1) & 0x3F));
        piece->timer = Rand_Next(1) & 0xF;
        ofs[0] = (s32) (Rand_Next(1) & 0x7F) - 64.0f;
        ofs[1] = (s32) (Rand_Next(1) & 0x3F) - 32.0f;
        ofs[2] = -1.0f * (s32) (Rand_Next(1) & 0x3F) - 32.0f;
        piece->pos[0] = obj->pos[0] + ofs[0];
        piece->pos[1] = obj->pos[1] + ofs[1];
        piece->pos[2] = obj->pos[2] + ofs[2];
        piece->rot[0] = Rand_Next(1);
        piece->rot[1] = Rand_Next(1);
        piece->rot[2] = Rand_Next(1);
        ofs[0] = 0.1f * (s32) (Rand_Next(1) & 0x3F) - 3.2f;
        ofs[1] = 0.0f;
        ofs[2] = 2.0f + 0.1f * (s32) (Rand_Next(1) & 0xF);
        Vec_RotateSelf(ofs, boneMtx);
        piece->vel[0] = ofs[0];
        piece->vel[1] = 0.0f;
        piece->vel[2] = ofs[2];
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            failed = 0;
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = FallingDebris_Draw;
            piece->alive = 1;
        } else {
            piece->node = NULL;
            piece->alive = 0;
        }
    }
    obj->pos[1] = em->floorY;
    if (failed != 0) {
        StageObj_Free(obj);
    }
}

/*
 * Moves a FallingDebris piece by its speed and lets it fall (1.4545 a frame).
 * obj is unused.
 */
void FallingDebris_MovePiece(StageObj *obj, FallingDebrisPiece *piece)
{
    piece->pos[0] += piece->vel[0];
    piece->pos[1] += piece->vel[1];
    piece->pos[2] += piece->vel[2];
    piece->vel[1] += -1.4545455f;
}

/*
 * Run state of a FallingDebris: each live piece waits its time, falls to the
 * floor height (obj->pos[1]), bounces up once with its sideways speed times
 * 1.8, and moves on for 100 frames; a shown piece is queued for drawing. Ends
 * when no piece is alive or the monster has left the current area. The piece
 * pointer moves on only past a live piece, so the pieces after a dead one are
 * not moved.
 */
void FallingDebris_Run(StageObj *obj)
{
    FallingDebrisPiece *piece;
    s16 i;
    s16 done;
    void *owner;

    i = 0;
    done = 1;
    piece = (FallingDebrisPiece *) obj->buf;
    owner = obj->owner;
    for (; i < 3; i++) {
        if (piece->alive) {
            done = 0;
            switch (piece->phase) {
            case 0:
                if (--piece->timer <= 0) {
                    piece->phase++;
                    piece->visible = 1;
                }
                break;
            case 1:
                FallingDebris_MovePiece(obj, piece);
                if (piece->pos[1] <= obj->pos[1]) {
                    piece->phase++;
                    piece->timer = 100;
                    piece->pos[1] = obj->pos[1];
                    piece->vel[0] *= 1.8f;
                    piece->vel[2] *= 1.8f;
                    piece->vel[1] = 3.0f + 0.15f * (s32) (Rand_Next(1) & 0x1F);
                }
                break;
            case 2:
                FallingDebris_MovePiece(obj, piece);
                if (--piece->timer <= 0) {
                    piece->alive = 0;
                }
                break;
            }
            if (piece->visible) {
                piece->node->pos[0] = piece->pos[0];
                piece->node->pos[1] = piece->pos[1];
                piece->node->pos[2] = piece->pos[2];
                OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
            }
            piece++;
        }
    }
    if (done != 0 || !(Obj_InCurrentArea(owner) & 0xFF)) {
        obj->state++;
    }
}

/*
 * End state of a FallingDebris: frees the pieces' draw slots and moves to the
 * free state.
 */
void FallingDebris_End(StageObj *obj)
{
    u8 *cursor = obj->buf;
    s16 i;

    obj->state++;
    for (i = 0; i < 3; i++) {
        if (*(s16 *) (cursor + 6) != -1) {
            DrawSlot_Free(*(s16 *) (cursor + 6));
        }
        cursor += 0x34;
    }
}

/* Free state of a FallingDebris: frees the object. */
void FallingDebris_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draw callback of one FallingDebris piece: mesh 43 (type 0) or 41 (the
 * others; type 2 tinted through RS_COLOR2) of the stage model set, at the
 * piece's position, rotation and size, with its materials' colors set to
 * white.
 */
void FallingDebris_Draw(DrawSlot *node)
{
    f32 mtx[16];
    f32 scaleMtx[16];
    StageObj *obj = node->owner;
    ModelMat *mats;
    ModelMatSet *model;
    ModelMat *mat;
    s32 i;
    FallingDebrisPiece *piece = (FallingDebrisPiece *) obj->buf + node->part;
    Model *set = gStageModelSet;

    if (set != NULL && set->active != 0) {
        switch (obj->type) {
        case 0:
            model = &set->matSets[43];
            Gs_SetState(0x67, -1);
            break;
        case 1:
        default:
            model = &set->matSets[41];
            Gs_SetState(0x67, -1);
            break;
        case 2:
            model = &set->matSets[41];
            Gs_SetState(0x67, 0xFFD08E65);
            break;
        }
        mats = set->mats;
        Mtx_Scale(scaleMtx, piece->scale, piece->scale, piece->scale);
        Mtx_Translation(mtx, piece->pos[0], piece->pos[1], piece->pos[2]);
        Mtx_SetRotXYZ(mtx, (6.2831855f * piece->rot[0]) / 65536.0f, (6.2831855f * piece->rot[1]) / 65536.0f,
                      (6.2831855f * piece->rot[2]) / 65536.0f);
        Mtx_Concat3x3(mtx, scaleMtx);
        Gs_SetState(0x1A, (s32) mtx);
        if (model != NULL && model->id != -1) {
            for (i = 0; i < model->count; i++) {
                mat = &mats[model->mat[i]];
                mat->rgb[0] = 1.0f;
                mat->rgb[1] = 1.0f;
                mat->rgb[2] = 1.0f;
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Creates the StunStars of a player in the current area: type 0, circling
 * bone 0x14 with the offset (0, 30, 25), at size 1.0.
 */
void StunStars_CreatePlayer(Player *pl, u8 objType)
{
    StageObj *obj;

    if (Obj_InCurrentArea(pl) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_STUN_STARS;
            obj->update = StunStars_Update;
            obj->draw = NULL;
            obj->owner = pl;
            obj->ownerIndex = pl->index;
            obj->type = objType;
            obj->pos[0] = 0.0f;
            obj->pos[1] = 30.0f;
            obj->pos[2] = 25.0f;
            obj->scale = 1.0f;
            obj->angle = 0x14;
        }
    }
}

/*
 * Creates StunStars for a monster when a player is in its area: type 1 (a
 * stun) or 2 (flash blindness), circling bone bone at height height, radius
 * 25 times scale, for frames frames.
 */
void StunStars_CreateEm(Em *em, u8 objType, s16 bone, s16 frames, f32 height, f32 scale)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_STUN_STARS;
            obj->update = StunStars_Update;
            obj->draw = NULL;
            obj->owner = em;
            obj->ownerIndex = em->index;
            obj->type = objType;
            obj->timer = frames;
            obj->pos[1] = height;
            obj->pos[0] = 0.0f;
            obj->pos[2] = 25.0f;
            obj->scale = scale;
            obj->angle = bone;
        }
    }
}

/*
 * Update of StunStars: runs its set-up, run, end and free states by
 * obj->state.
 */
void StunStars_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        StunStars_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        StunStars_Run(obj);
        break;
    case STAGEOBJ_END:
        StunStars_End(obj);
        break;
    case STAGEOBJ_FREE:
        StunStars_Free(obj);
        break;
    }
}

/*
 * Set-up state of StunStars: records the owner state the stars last for (type
 * 0: the player's PL_GROUP_REACT/PL_REACT_STUN; a monster: its current state
 * group and state, and the frame limit), the bone and the circle's offset
 * (its radius times the size), a random start angle, and takes five draw
 * slots.
 */
void StunStars_Setup(StageObj *obj)
{
    StunWork *w = (StunWork *) obj->buf;
    s16 i;

    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    case 0:
        w->group = 2;
        w->state = 0x13;
        break;
    case 1:
        w->limit = obj->timer;
        w->group = ((Em *) obj->owner)->stateGroup;
        w->state = ((Em *) obj->owner)->state;
        break;
    case 2:
        w->limit = obj->timer;
        w->group = ((Em *) obj->owner)->stateGroup;
        w->state = ((Em *) obj->owner)->state;
        break;
    }
    w->bone = obj->angle;
    Vec_Copy(w->ofs, obj->pos);
    w->ofs[2] *= obj->scale;
    obj->timer = 0;
    obj->angle = Rand_Next(1);
    for (i = 0; i < 5; i++) {
        w->slot[i] = DrawSlot_Alloc();
        if (w->slot[i] != -1) {
            w->node[i] = DrawSlot_Get(w->slot[i]);
            w->node[i]->owner = obj;
            w->node[i]->part = i;
            w->node[i]->draw = StunStars_Draw;
        } else {
            w->node[i] = NULL;
        }
    }
}

/*
 * Run state of StunStars: ends when the owner is inactive or leaves the
 * recorded state (type 2: when the monster is defeated or its blindTimer runs
 * out), and for types 1 and 2 after the frame limit. A player gets as many
 * stars as his stateVar1 (at most five; none ends it) and sound 0x38 every 23
 * frames, a monster five; they turn around the bone 0x800 a frame, evenly
 * spaced, and are queued for drawing.
 */
void StunStars_Run(StageObj *obj)
{
    StunWork *w = (StunWork *) obj->buf;
    Em *em = obj->owner;
    s16 count;
    s16 i;
    f32 boneMtx[16];

    if (obj->type == 2) {
        if (em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (em->stateGroup == 5 || em->blindTimer <= 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
    } else if (em->active == 0 || Player_IsState(em, w->group, w->state) == 0) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    obj->timer++;
    switch (obj->type) {
    case 0:
        count = em->stateVar1;
        if (obj->timer % 23 == 1) {
            Sound_PlaySeAtPosChecked(1, 0x38, 0, &em->posX, 1, 0);
        }
        break;
    case 1:
    case 2:
        if (obj->timer > w->limit) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        count = 5;
        break;
    }
    if (count == 0) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    if (count > 5) {
        count = 5;
    }
    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, w->bone)));
    Mtx_GetTrans(obj->pos, boneMtx);
    obj->angle += 0x800;
    for (i = 0; i < count; i++) {
        if (w->node[i] != NULL) {
            Vec_Copy(w->node[i]->pos, w->ofs);
            Vec_RotateY(w->node[i]->pos, (6.2831855f * (obj->angle + i * (0x10000 / count))) / 65536.0f);
            Vec_RotateSelf(w->node[i]->pos, boneMtx);
            w->node[i]->pos[0] += obj->pos[0];
            w->node[i]->pos[1] += obj->pos[1];
            w->node[i]->pos[2] += obj->pos[2];
            OT_AddByDepth(gDrawOt32, w->node[i], 0x20, 0);
        }
    }
}

typedef struct StunSlotsView { s16 id[5]; u8 unkA[0xC - 0xA]; s32 h[5]; } StunSlotsView;

/*
 * End state of StunStars: frees the five draw slots and moves to the free
 * state.
 */
void StunStars_End(StageObj *obj)
{
    StunSlotsView *slots = (StunSlotsView *) obj->buf;
    s16 i;

    obj->state++;
    for (i = 0; i < 5; i++) {
        if (slots->h[i] != 0) {
            DrawSlot_Free(slots->id[i]);
        }
    }
}

/* Free state of StunStars: frees the object. */
void StunStars_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draw callback of one star: mesh 118 of the common model set at the object's
 * size, facing the camera, added (Effect_DrawModelAdd).
 */
void StunStars_Draw(DrawSlot *node)
{
    Model *mdl = gCommonModels[0];
    StageObj *obj = node->owner;
    ModelMat *tex;
    ModelMatSet *mesh;
    f32 mtx[16];

    if (mdl == NULL || mdl->active == 0) {
        return;
    }
    tex = mdl->mats;
    mesh = &mdl->matSets[118];
    Mtx_Scale(mtx, obj->scale, obj->scale, obj->scale);
    Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
    Mtx_Concat3x3(mtx, gCamMatrix);
    Gs_SetState(0x67, -1);
    Effect_DrawModelAdd(mesh, mtx, tex);
}

/*
 * Update of MashPieces: runs its set-up, run, end and free states by
 * obj->state.
 */
void MashDebris_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        MashDebris_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        MashDebris_Run(obj);
        break;
    case STAGEOBJ_END:
        MashDebris_End(obj);
        break;
    case STAGEOBJ_FREE:
        MashDebris_Free(obj);
        break;
    }
}

/*
 * Set-up state of MashPieces; ends at once without owner. One random number
 * is drawn and dropped first. At the owner's bone 2, type 0
 */
void MashDebris_Setup(StageObj *obj)
{
    MashDebrisPiece *piece = (MashDebrisPiece *) obj->buf;
    f32 bonePos[3];
    f32 launchVel[3];
    u8 i;
    u16 yaw;
    u16 bits;
    if (obj->owner == NULL) {
        obj->state = STAGEOBJ_END;
        obj->visible = 0;
        return;
    }
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    Em_GetBonePos(obj->owner, 2, bonePos);
    Rand_Next(1);
    switch (obj->type) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x8D, 0, bonePos, 1, 0);
        break;
    }
    for (i = 0; i < MashDebris_PartCounts[obj->type]; i++) {
        switch (obj->type) {
        case 0:
            memset(piece, 0, 0x40);
            piece->alpha = 0xFF;
            piece->model = MashDebris_Models[i];
            piece->life = MashDebris_Lives[i];
            switch (i) {
            case 0:
            case 4:
                piece->ground = ((Em *) obj->owner)->floorY;
                piece->pos[0] = -20.0f + Rand_Next(1) % 40;
                piece->pos[1] = -40.0f + Rand_Next(1) % 80;
                piece->pos[2] = -20.0f + Rand_Next(1) % 40;
                piece->rot[0] = Rand_Next(1) % 0xE39;
                piece->rot[1] = Rand_Next(1);
                if (i == 0) {
                    yaw = piece->rot[1];
                }
                if (i == 4) {
                    piece->rot[1] = yaw + 0x6000 + (Rand_Next(1) & 0x1FFF);
                }
                piece->rot[2] = Rand_Next(1) % 0x1C72;
                piece->scale[0] = piece->scale[1] = piece->scale[2] = 1.0f + 0.01f * (Rand_Next(1) % 20);
                piece->fall = -7.0f;
                bits = Rand_Next(1);
                obj->var0 = bits & 1;
                obj->var1 = (bits >> 1) & 1;
                launchVel[0] = 0.0f;
                launchVel[1] = 0.0f;
                launchVel[2] = 10.0f + (Rand_Next(1) & 3);
                Vec_RotateY(launchVel, (6.2831855f * piece->rot[1]) / 65536.0f);
                piece->vel[0] = launchVel[0];
                piece->vel[1] = launchVel[1];
                piece->vel[2] = launchVel[2];
                piece->rot[1] -= 0x4000;
                break;
            case 1:
                piece->scale[2] = 1.0f;
                break;
            case 2:
                piece->rot[2] = Rand_Next(1);
                piece->scale[0] = piece->scale[1] = piece->scale[2] = 1.0f;
                break;
            case 3:
                piece->rot[2] = Rand_Next(1);
                piece->scale[0] = 0.5f;
                piece->scale[1] = 1.0f;
                obj->var0 = Rand_Next(1) & 1;
                piece->rot[1] = Rand_Next(1);
                launchVel[0] = 0.0f;
                launchVel[1] = 0.0f;
                launchVel[2] = 10.0f + (Rand_Next(1) & 3);
                Vec_RotateY(launchVel, (6.2831855f * piece->rot[1]) / 65536.0f);
                piece->vel[0] = launchVel[0];
                piece->vel[1] = launchVel[1];
                piece->vel[2] = launchVel[2];
                piece->fall = -7.0f - (Rand_Next(1) & 3);
                piece->pos[0] = -20.0f + Rand_Next(1) % 40;
                piece->pos[1] = -80.0f + Rand_Next(1) % 160;
                piece->pos[2] = -20.0f + Rand_Next(1) % 40;
                break;
            }
            piece->pos[0] += bonePos[0];
            piece->pos[1] += bonePos[1];
            piece->pos[2] += bonePos[2];
            break;
        case 1:
            memset(piece, 0, 0x40);
            piece->alpha = 0xFF;
            piece->model = (Rand_Next(1) & 1) + 0x92;
            piece->t = -(Rand_Next(1) % 5);
            piece->pos[2] = 20.0f + (u32) (Rand_Next(1) % 10U);
            Vec_RotateY(piece->pos, (6.2831855f * (s32) Rand_Next(1)) / 65536.0f);
            piece->rot[2] = Rand_Next(1);
            piece->fall = 1.0f;
            obj->var0 = Rand_Next(1) & 1;
            piece->life = 10;
            piece->scale[0] = piece->scale[1] = piece->scale[2] = 1.0f;
            break;
        }
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = (DrawSlotFn)MashDebris_Draw; /* MashDebris_Draw is defined taking void *: with a DrawSlot * parameter its code uses other registers */
        } else {
            piece->node = NULL;
        }
        piece++;
    }
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    MashDebris_Run(obj);
}

/* The file's data. */
extern u32 D_008F5CD8[];
u32 ShotFlash_Curves_0[36] = {
#include "game/008D0820.inc"
};
u32 ShotFlash_Curves_1[36] = {
#include "game/008D08B0.inc"
};
u32 ShotFlash_Curves_4[16] = {
#include "game/008D0940.inc"
};
u32 ShotFlash_Curves_8[16] = {
#include "game/008D0980.inc"
};
u32 ShotFlash_Curves_12[16] = {
#include "game/008D09C0.inc"
};
u32 ShotFlash_Curves_2[12] = {
#include "game/008D0A00.inc"
};
u32 ShotFlash_Curves_6[12] = {
#include "game/008D0A30.inc"
};
u32 ShotFlash_Curves_10[12] = {
#include "game/008D0A60.inc"
};
u32 ShotFlash_Curves_5[8] = {
#include "game/008D0A90.inc"
};
u32 ShotFlash_Curves_9[8] = {
#include "game/008D0AB0.inc"
};
u32 ShotFlash_Curves_13[8] = {
#include "game/008D0AD0.inc"
};
u32 ShotFlash_Curves_15[12] = {
#include "game/008D0AF0.inc"
};
u32 ShotFlash_Curves_14[12] = {
#include "game/008D0B20.inc"
};
u32 ShotFlash_Curves_16[8] = {
#include "game/008D0B50.inc"
};
u32 ShotFlash_Curves_17[20] = {
#include "game/008D0B70.inc"
};
u32 ShotFlash_Curves_18[8] = {
#include "game/008D0BC0.inc"
};
u32 ShotFlash_Curves_19[24] = {
#include "game/008D0BE0.inc"
};
u32 ShotFlash_Curves_21[28] = {
#include "game/008D0C40.inc"
};
u32 ShotFlash_Curves_22[20] = {
#include "game/008D0CB0.inc"
};
u32 ShotFlash_Curves_23[16] = {
#include "game/008D0D00.inc"
};
u32 ShotFlash_Curves_24[12] = {
#include "game/008D0D40.inc"
};
u32 ShotFlash_Curves_25[20] = {
#include "game/008D0D70.inc"
};
u32 ShotFlash_Curves_26[20] = {
#include "game/008D0DC0.inc"
};
u32 ShotFlash_Curves_29[12] = {
#include "game/008D0E10.inc"
};
u32 ShotFlash_Curves_30[16] = {
#include "game/008D0E40.inc"
};
u32 ShotFlash_Curves_31[18] = {
#include "game/008D0E80.inc"
};
s16 ShotFlash_BurstDelays[6] = {
#include "game/008D0EC8.inc"
};
s16 ShotFlash_BurstDurations[6] = {
#include "game/008D0ED8.inc"
};
u8 ShotFlash_GunlanceShellDelays[8] = {
#include "game/008D0EE8.inc"
};
void *ShotFlash_Curves[34] = {
    (void *)ShotFlash_Curves_0, (void *)ShotFlash_Curves_1, (void *)ShotFlash_Curves_2, (void *)ShotFlash_Curves_2,
    (void *)ShotFlash_Curves_4, (void *)ShotFlash_Curves_5, (void *)ShotFlash_Curves_6, (void *)ShotFlash_Curves_6,
    (void *)ShotFlash_Curves_8, (void *)ShotFlash_Curves_9, (void *)ShotFlash_Curves_10, (void *)ShotFlash_Curves_10,
    (void *)ShotFlash_Curves_12, (void *)ShotFlash_Curves_13, (void *)ShotFlash_Curves_14, (void *)ShotFlash_Curves_15,
    (void *)ShotFlash_Curves_16, (void *)ShotFlash_Curves_17, (void *)ShotFlash_Curves_18, (void *)ShotFlash_Curves_19,
    (void *)ShotFlash_Curves_19, (void *)ShotFlash_Curves_21, (void *)ShotFlash_Curves_22, (void *)ShotFlash_Curves_23,
    (void *)ShotFlash_Curves_24, (void *)ShotFlash_Curves_25, (void *)ShotFlash_Curves_26, (void *)ShotFlash_Curves_26,
    (void *)ShotFlash_Curves_26, (void *)ShotFlash_Curves_29, (void *)ShotFlash_Curves_30, (void *)ShotFlash_Curves_31,
    (void *)ShotFlash_Curves_31, (void *)ShotFlash_Curves_31,
};
s16 ShotFlash_FirePuffDurations[6] = {
#include "game/008D0F78.inc"
};
f32 ShotFlash_FirePuffScales[3] = {
#include "game/008D0F88.inc"
};
f32 ShotFlash_FirePuffSpin[3] = {
#include "game/008D0F98.inc"
};
u32 ShotFlash_Tbls_0_0[8] = {
#include "game/008D0FB0.inc"
};
u32 ShotFlash_Tbls_2_0[8] = {
#include "game/008D0FD0.inc"
};
u32 ShotFlash_Tbls_3_0[12] = {
#include "game/008D0FF0.inc"
};
u32 ShotFlash_Tbls_4_0[8] = {
#include "game/008D1020.inc"
};
u32 ShotFlash_Tbls_0_1[8] = {
#include "game/008D1040.inc"
};
u32 ShotFlash_Tbls_2_1[8] = {
#include "game/008D1060.inc"
};
u32 ShotFlash_Tbls_3_1[12] = {
#include "game/008D1080.inc"
};
u32 ShotFlash_Tbls_4_1[8] = {
#include "game/008D10B0.inc"
};
u32 ShotFlash_Tbls_0_2[8] = {
#include "game/008D10D0.inc"
};
u32 ShotFlash_Tbls_2_2[8] = {
#include "game/008D10F0.inc"
};
u32 ShotFlash_Tbls_3_2[12] = {
#include "game/008D1110.inc"
};
u32 ShotFlash_Tbls_4_2[8] = {
#include "game/008D1140.inc"
};
u32 ShotFlash_Tbls_0_3[8] = {
#include "game/008D1160.inc"
};
u32 ShotFlash_Tbls_2_3[8] = {
#include "game/008D1180.inc"
};
u32 ShotFlash_Tbls_3_3[12] = {
#include "game/008D11A0.inc"
};
u32 ShotFlash_Tbls_4_3[8] = {
#include "game/008D11D0.inc"
};
u32 ShotFlash_Tbls_0_4[8] = {
#include "game/008D11F0.inc"
};
u32 ShotFlash_Tbls_2_4[8] = {
#include "game/008D1210.inc"
};
u32 ShotFlash_Tbls_3_4[12] = {
#include "game/008D1230.inc"
};
u32 ShotFlash_Tbls_4_4[8] = {
#include "game/008D1260.inc"
};
u32 ShotFlash_Tbls_0_5[8] = {
#include "game/008D1280.inc"
};
u32 ShotFlash_Tbls_2_5[8] = {
#include "game/008D12A0.inc"
};
u32 ShotFlash_Tbls_3_5[12] = {
#include "game/008D12C0.inc"
};
u32 ShotFlash_Tbls_4_5[8] = {
#include "game/008D12F0.inc"
};
u32 ShotFlash_Tbls_0_6[8] = {
#include "game/008D1310.inc"
};
u32 ShotFlash_Tbls_2_6[8] = {
#include "game/008D1330.inc"
};
u32 ShotFlash_Tbls_3_6[12] = {
#include "game/008D1350.inc"
};
u32 ShotFlash_Tbls_4_6[8] = {
#include "game/008D1380.inc"
};
u32 ShotFlash_Tbls_0_7[8] = {
#include "game/008D13A0.inc"
};
u32 ShotFlash_Tbls_2_7[8] = {
#include "game/008D13C0.inc"
};
u32 ShotFlash_Tbls_3_7[12] = {
#include "game/008D13E0.inc"
};
u32 ShotFlash_Tbls_4_7[8] = {
#include "game/008D1410.inc"
};
u32 ShotFlash_Tbls_0_8[8] = {
#include "game/008D1430.inc"
};
u32 ShotFlash_Tbls_2_8[8] = {
#include "game/008D1450.inc"
};
u32 ShotFlash_Tbls_3_8[12] = {
#include "game/008D1470.inc"
};
u32 ShotFlash_Tbls_4_8[8] = {
#include "game/008D14A0.inc"
};
u32 ShotFlash_Tbls_0_9[8] = {
#include "game/008D14C0.inc"
};
u32 ShotFlash_Tbls_2_9[8] = {
#include "game/008D14E0.inc"
};
u32 ShotFlash_Tbls_3_9[12] = {
#include "game/008D1500.inc"
};
u32 ShotFlash_Tbls_4_9[8] = {
#include "game/008D1530.inc"
};
u32 ShotFlash_Tbls_0_10[8] = {
#include "game/008D1550.inc"
};
u32 ShotFlash_Tbls_2_10[8] = {
#include "game/008D1570.inc"
};
u32 ShotFlash_Tbls_3_10[12] = {
#include "game/008D1590.inc"
};
u32 ShotFlash_Tbls_4_10[8] = {
#include "game/008D15C0.inc"
};
u32 ShotFlash_Tbls_0_11[8] = {
#include "game/008D15E0.inc"
};
u32 ShotFlash_Tbls_2_11[8] = {
#include "game/008D1600.inc"
};
u32 ShotFlash_Tbls_3_11[12] = {
#include "game/008D1620.inc"
};
u32 ShotFlash_Tbls_4_11[8] = {
#include "game/008D1650.inc"
};
u32 ShotFlash_Tbls_0_12[8] = {
#include "game/008D1670.inc"
};
u32 ShotFlash_Tbls_2_12[8] = {
#include "game/008D1690.inc"
};
u32 ShotFlash_Tbls_3_12[12] = {
#include "game/008D16B0.inc"
};
u32 ShotFlash_Tbls_4_12[8] = {
#include "game/008D16E0.inc"
};
u32 ShotFlash_Tbls_0_13[8] = {
#include "game/008D1700.inc"
};
u32 ShotFlash_Tbls_2_13[8] = {
#include "game/008D1720.inc"
};
u32 ShotFlash_Tbls_3_13[12] = {
#include "game/008D1740.inc"
};
u32 ShotFlash_Tbls_4_13[8] = {
#include "game/008D1770.inc"
};
u8 *ShotFlash_Tbls_0[16] = {
    (u8 *)ShotFlash_Tbls_0_0, (u8 *)ShotFlash_Tbls_0_1, (u8 *)ShotFlash_Tbls_0_2, (u8 *)ShotFlash_Tbls_0_3,
    (u8 *)ShotFlash_Tbls_0_4, (u8 *)ShotFlash_Tbls_0_5, (u8 *)ShotFlash_Tbls_0_6, (u8 *)ShotFlash_Tbls_0_7,
    (u8 *)ShotFlash_Tbls_0_8, (u8 *)ShotFlash_Tbls_0_9, (u8 *)ShotFlash_Tbls_0_10, (u8 *)ShotFlash_Tbls_0_11,
    (u8 *)ShotFlash_Tbls_0_12, (u8 *)ShotFlash_Tbls_0_13, NULL, NULL,
};
u8 *ShotFlash_Tbls_2[16] = {
    (u8 *)ShotFlash_Tbls_2_0, (u8 *)ShotFlash_Tbls_2_1, (u8 *)ShotFlash_Tbls_2_2, (u8 *)ShotFlash_Tbls_2_3,
    (u8 *)ShotFlash_Tbls_2_4, (u8 *)ShotFlash_Tbls_2_5, (u8 *)ShotFlash_Tbls_2_6, (u8 *)ShotFlash_Tbls_2_7,
    (u8 *)ShotFlash_Tbls_2_8, (u8 *)ShotFlash_Tbls_2_9, (u8 *)ShotFlash_Tbls_2_10, (u8 *)ShotFlash_Tbls_2_11,
    (u8 *)ShotFlash_Tbls_2_12, (u8 *)ShotFlash_Tbls_2_13, NULL, NULL,
};
u8 *ShotFlash_Tbls_3[16] = {
    (u8 *)ShotFlash_Tbls_3_0, (u8 *)ShotFlash_Tbls_3_1, (u8 *)ShotFlash_Tbls_3_2, (u8 *)ShotFlash_Tbls_3_3,
    (u8 *)ShotFlash_Tbls_3_4, (u8 *)ShotFlash_Tbls_3_5, (u8 *)ShotFlash_Tbls_3_6, (u8 *)ShotFlash_Tbls_3_7,
    (u8 *)ShotFlash_Tbls_3_8, (u8 *)ShotFlash_Tbls_3_9, (u8 *)ShotFlash_Tbls_3_10, (u8 *)ShotFlash_Tbls_3_11,
    (u8 *)ShotFlash_Tbls_3_12, (u8 *)ShotFlash_Tbls_3_13, NULL, NULL,
};
u8 *ShotFlash_Tbls_4[16] = {
    (u8 *)ShotFlash_Tbls_4_0, (u8 *)ShotFlash_Tbls_4_1, (u8 *)ShotFlash_Tbls_4_2, (u8 *)ShotFlash_Tbls_4_3,
    (u8 *)ShotFlash_Tbls_4_4, (u8 *)ShotFlash_Tbls_4_5, (u8 *)ShotFlash_Tbls_4_6, (u8 *)ShotFlash_Tbls_4_7,
    (u8 *)ShotFlash_Tbls_4_8, (u8 *)ShotFlash_Tbls_4_9, (u8 *)ShotFlash_Tbls_4_10, (u8 *)ShotFlash_Tbls_4_11,
    (u8 *)ShotFlash_Tbls_4_12, (u8 *)ShotFlash_Tbls_4_13, NULL, NULL,
};
u8* *ShotFlash_ColorFlashKeys[6] = {
    ShotFlash_Tbls_0, NULL, ShotFlash_Tbls_2, ShotFlash_Tbls_3,
    ShotFlash_Tbls_4, NULL,
};
s16 ShotFlash_ColorFlashDurations[6] = {
#include "game/008D18A8.inc"
};
u8 ShotFlash_ColorKeysA_0[48] = {
#include "game/008D18C0.inc"
};
u8 ShotFlash_ColorKeysA_1[48] = {
#include "game/008D18F0.inc"
};
u8 ShotFlash_ColorKeysA_2[48] = {
#include "game/008D1920.inc"
};
u8 ShotFlash_ColorKeysA_3[48] = {
#include "game/008D1950.inc"
};
u8 ShotFlash_ColorKeysA_4[48] = {
#include "game/008D1980.inc"
};
u8 ShotFlash_ColorKeysA_5[48] = {
#include "game/008D19B0.inc"
};
u8 ShotFlash_ColorKeysA_6[48] = {
#include "game/008D19E0.inc"
};
u8 ShotFlash_ColorKeysA_7[48] = {
#include "game/008D1A10.inc"
};
u8 ShotFlash_ColorKeysA_8[48] = {
#include "game/008D1A40.inc"
};
u8 ShotFlash_ColorKeysA_9[48] = {
#include "game/008D1A70.inc"
};
u8 ShotFlash_ColorKeysA_12[48] = {
#include "game/008D1AA0.inc"
};
u8 *ShotFlash_ColorSmokeKeys[16] = {
    ShotFlash_ColorKeysA_0, ShotFlash_ColorKeysA_1, ShotFlash_ColorKeysA_2, ShotFlash_ColorKeysA_3,
    ShotFlash_ColorKeysA_4, ShotFlash_ColorKeysA_5, ShotFlash_ColorKeysA_6, ShotFlash_ColorKeysA_7,
    ShotFlash_ColorKeysA_8, ShotFlash_ColorKeysA_9, ShotFlash_ColorKeysA_9, ShotFlash_ColorKeysA_9,
    ShotFlash_ColorKeysA_12, NULL, NULL, NULL,
};
u8 ShotFlash_ColorKeysB_0[48] = {
#include "game/008D1B10.inc"
};
u8 ShotFlash_ColorKeysB_1[48] = {
#include "game/008D1B40.inc"
};
u8 ShotFlash_ColorKeysB_2[48] = {
#include "game/008D1B70.inc"
};
u8 ShotFlash_ColorKeysB_3[48] = {
#include "game/008D1BA0.inc"
};
u8 ShotFlash_ColorKeysB_4[48] = {
#include "game/008D1BD0.inc"
};
u8 ShotFlash_ColorKeysB_5[48] = {
#include "game/008D1C00.inc"
};
u8 ShotFlash_ColorKeysB_6[48] = {
#include "game/008D1C30.inc"
};
u8 ShotFlash_ColorKeysB_7[48] = {
#include "game/008D1C60.inc"
};
u8 ShotFlash_ColorKeysB_8[48] = {
#include "game/008D1C90.inc"
};
u8 *ShotFlash_ImpactKeys[12] = {
    ShotFlash_ColorKeysB_0, ShotFlash_ColorKeysB_1, ShotFlash_ColorKeysB_2, ShotFlash_ColorKeysB_3,
    ShotFlash_ColorKeysB_4, ShotFlash_ColorKeysB_5, ShotFlash_ColorKeysB_6, ShotFlash_ColorKeysB_7,
    ShotFlash_ColorKeysB_8, NULL, NULL, NULL,
};
u8 ShotFlash_ColorKeysC_0[32] = {
#include "game/008D1CF0.inc"
};
u8 ShotFlash_ColorKeysC_1[32] = {
#include "game/008D1D10.inc"
};
u8 ShotFlash_ColorKeysC_2[48] = {
#include "game/008D1D30.inc"
};
u8 ShotFlash_ColorKeysC_3[32] = {
#include "game/008D1D60.inc"
};
u8 *ShotFlash_BurstKeys[4] = {
    ShotFlash_ColorKeysC_0, ShotFlash_ColorKeysC_1, ShotFlash_ColorKeysC_2, ShotFlash_ColorKeysC_3,
};
s16 ShotFlash_PartCounts[16] = {
#include "game/008D1D90.inc"
};
s16 ShotFlash_Lifetimes[16] = {
#include "game/008D1DB0.inc"
};
s16 ShotFlash_CurveIdx[16] = {
#include "game/008D1DD0.inc"
};
f32 AlertMark_OffsetCommon[4] = {
#include "game/008D1DF0.inc"
};
f32 AlertMark_OffsetKutKu[4] = {
#include "game/008D1E00.inc"
};
f32 AlertMark_OffsetDiablos[4] = {
#include "game/008D1E10.inc"
};
f32 *AlertMark_OffsetsByEmKind[74] = {
    (f32 *)D_008F5CD8, AlertMark_OffsetCommon, AlertMark_OffsetCommon, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetKutKu, (f32 *)D_008F5CD8,
    AlertMark_OffsetCommon, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetCommon,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetDiablos, AlertMark_OffsetCommon,
    (f32 *)D_008F5CD8, AlertMark_OffsetDiablos, AlertMark_OffsetCommon, AlertMark_OffsetCommon,
    AlertMark_OffsetCommon, AlertMark_OffsetKutKu, AlertMark_OffsetDiablos, AlertMark_OffsetCommon,
    AlertMark_OffsetCommon, AlertMark_OffsetCommon, AlertMark_OffsetDiablos, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetCommon, (f32 *)D_008F5CD8,
    AlertMark_OffsetCommon, AlertMark_OffsetCommon, AlertMark_OffsetKutKu, AlertMark_OffsetCommon,
    AlertMark_OffsetKutKu, AlertMark_OffsetCommon, AlertMark_OffsetCommon, AlertMark_OffsetDiablos,
    AlertMark_OffsetDiablos, AlertMark_OffsetCommon, AlertMark_OffsetKutKu, AlertMark_OffsetDiablos,
    (f32 *)D_008F5CD8, AlertMark_OffsetCommon, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetDiablos,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetCommon,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, AlertMark_OffsetCommon,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
};
f32 D_008D1F48[3] = {
#include "game/008D1F48.inc"
};
f32 *AlertMark_PlayerOffset = D_008D1F48;
f32 AlertMark_ScaleKeys[32] = {
#include "game/008D1F60.inc"
};
f32 AlertMark_MoveKeys[36] = {
#include "game/008D1FE0.inc"
};
f32 AlertMark_FixedMarkScaleKeys[22] = {
#include "game/008D2070.inc"
};
s16 AlertMark_MarkMeshes[4] = {
#include "game/008D20C8.inc"
};
u32 DustCloud_Curves_0[20] = {
#include "game/008D20D0.inc"
};
u32 DustCloud_Curves_1[12] = {
#include "game/008D2120.inc"
};
u32 DustCloud_Curves_2[8] = {
#include "game/008D2150.inc"
};
u32 DustCloud_Curves_3[20] = {
#include "game/008D2170.inc"
};
u32 DustCloud_Curves_4[20] = {
#include "game/008D21C0.inc"
};
u32 DustCloud_Curves_5[10] = {
#include "game/008D2210.inc"
};
s16 DustCloud_DelayLists_1[6] = {
#include "game/008D2238.inc"
};
u32 DustCloud_Curves_6[20] = {
#include "game/008D2250.inc"
};
u32 DustCloud_Curves_9[20] = {
#include "game/008D22A0.inc"
};
u32 DustCloud_Curves_12[24] = {
#include "game/008D22F0.inc"
};
u32 DustCloud_Curves_15[20] = {
#include "game/008D2350.inc"
};
u32 DustCloud_Curves_7[16] = {
#include "game/008D23A0.inc"
};
u32 DustCloud_Curves_10[16] = {
#include "game/008D23E0.inc"
};
u32 DustCloud_Curves_13[24] = {
#include "game/008D2420.inc"
};
u32 DustCloud_Curves_16[20] = {
#include "game/008D2480.inc"
};
u32 DustCloud_Curves_8[8] = {
#include "game/008D24D0.inc"
};
u32 DustCloud_Curves_11[8] = {
#include "game/008D24F0.inc"
};
u32 DustCloud_Curves_14[8] = {
#include "game/008D2510.inc"
};
u32 DustCloud_Curves_17[10] = {
#include "game/008D2530.inc"
};
s16 DustCloud_DelayLists_2[4] = {
#include "game/008D2558.inc"
};
u16 DustCloud_DurLists_2[8] = {
#include "game/008D2560.inc"
};
u32 DustCloud_Curves_18[24] = {
#include "game/008D2570.inc"
};
u32 DustCloud_Curves_21[24] = {
#include "game/008D25D0.inc"
};
u32 DustCloud_Curves_19[12] = {
#include "game/008D2630.inc"
};
u32 DustCloud_Curves_22[16] = {
#include "game/008D2660.inc"
};
u32 DustCloud_Curves_20[12] = {
#include "game/008D26A0.inc"
};
u32 DustCloud_Curves_23[10] = {
#include "game/008D26D0.inc"
};
s16 DustCloud_DelayLists_4[4] = {
#include "game/008D26F8.inc"
};
u16 DustCloud_DurLists_4[8] = {
#include "game/008D2700.inc"
};
u32 DustCloud_Curves_24[24] = {
#include "game/008D2710.inc"
};
u32 DustCloud_Curves_27[24] = {
#include "game/008D2770.inc"
};
u32 DustCloud_Curves_30[24] = {
#include "game/008D27D0.inc"
};
u32 DustCloud_Curves_33[24] = {
#include "game/008D2830.inc"
};
u32 DustCloud_Curves_36[24] = {
#include "game/008D2890.inc"
};
u32 DustCloud_Curves_39[24] = {
#include "game/008D28F0.inc"
};
u32 DustCloud_Curves_42[24] = {
#include "game/008D2950.inc"
};
u32 DustCloud_Curves_25[16] = {
#include "game/008D29B0.inc"
};
u32 DustCloud_Curves_28[16] = {
#include "game/008D29F0.inc"
};
u32 DustCloud_Curves_31[24] = {
#include "game/008D2A30.inc"
};
u32 DustCloud_Curves_34[24] = {
#include "game/008D2A90.inc"
};
u32 DustCloud_Curves_37[24] = {
#include "game/008D2AF0.inc"
};
u32 DustCloud_Curves_40[20] = {
#include "game/008D2B50.inc"
};
u32 DustCloud_Curves_43[20] = {
#include "game/008D2BA0.inc"
};
u32 DustCloud_Curves_26[12] = {
#include "game/008D2BF0.inc"
};
u32 DustCloud_Curves_29[12] = {
#include "game/008D2C20.inc"
};
u32 DustCloud_Curves_32[12] = {
#include "game/008D2C50.inc"
};
u32 DustCloud_Curves_35[12] = {
#include "game/008D2C80.inc"
};
u32 DustCloud_Curves_38[12] = {
#include "game/008D2CB0.inc"
};
u32 DustCloud_Curves_41[12] = {
#include "game/008D2CE0.inc"
};
u32 DustCloud_Curves_44[20] = {
#include "game/008D2D10.inc"
};
u32 DustCloud_Curves_45[24] = {
#include "game/008D2D60.inc"
};
u32 DustCloud_Curves_48[24] = {
#include "game/008D2DC0.inc"
};
u32 DustCloud_Curves_51[24] = {
#include "game/008D2E20.inc"
};
u32 DustCloud_Curves_46[20] = {
#include "game/008D2E80.inc"
};
u32 DustCloud_Curves_49[20] = {
#include "game/008D2ED0.inc"
};
u32 DustCloud_Curves_52[20] = {
#include "game/008D2F20.inc"
};
u32 DustCloud_Curves_47[8] = {
#include "game/008D2F70.inc"
};
u32 DustCloud_Curves_50[8] = {
#include "game/008D2F90.inc"
};
u32 DustCloud_Curves_53[8] = {
#include "game/008D2FB0.inc"
};
s16 DustCloud_DelayLists_6[4] = {
#include "game/008D2FD0.inc"
};
u16 DustCloud_DurLists_6[4] = {
#include "game/008D2FD8.inc"
};
u32 DustCloud_Curves_54[24] = {
#include "game/008D2FE0.inc"
};
u32 DustCloud_Curves_55[8] = {
#include "game/008D3040.inc"
};
u32 DustCloud_Curves_56[28] = {
#include "game/008D3060.inc"
};
u32 DustCloud_Curves_59[28] = {
#include "game/008D30D0.inc"
};
u32 DustCloud_Curves_62[28] = {
#include "game/008D3140.inc"
};
u32 DustCloud_Curves_65[28] = {
#include "game/008D31B0.inc"
};
u32 DustCloud_Curves_68[28] = {
#include "game/008D3220.inc"
};
u32 DustCloud_Curves_57[28] = {
#include "game/008D3290.inc"
};
u32 DustCloud_Curves_60[28] = {
#include "game/008D3300.inc"
};
u32 DustCloud_Curves_63[28] = {
#include "game/008D3370.inc"
};
u32 DustCloud_Curves_66[16] = {
#include "game/008D33E0.inc"
};
u32 DustCloud_Curves_69[16] = {
#include "game/008D3420.inc"
};
u32 DustCloud_Curves_58[8] = {
#include "game/008D3460.inc"
};
u32 DustCloud_Curves_61[8] = {
#include "game/008D3480.inc"
};
u32 DustCloud_Curves_64[8] = {
#include "game/008D34A0.inc"
};
u32 DustCloud_Curves_67[8] = {
#include "game/008D34C0.inc"
};
u32 DustCloud_Curves_70[8] = {
#include "game/008D34E0.inc"
};
s16 DustCloud_DelayLists_8[8] = {
#include "game/008D3500.inc"
};
u16 DustCloud_DurLists_8[8] = {
#include "game/008D3510.inc"
};
u32 DustCloud_Curves_71[16] = {
#include "game/008D3520.inc"
};
u32 DustCloud_Curves_74[16] = {
#include "game/008D3560.inc"
};
u32 DustCloud_Curves_80[16] = {
#include "game/008D35A0.inc"
};
u32 DustCloud_Curves_86[16] = {
#include "game/008D35E0.inc"
};
u32 DustCloud_Curves_72[12] = {
#include "game/008D3620.inc"
};
u32 DustCloud_Curves_75[12] = {
#include "game/008D3650.inc"
};
u32 DustCloud_Curves_78[12] = {
#include "game/008D3680.inc"
};
u32 DustCloud_Curves_81[12] = {
#include "game/008D36B0.inc"
};
u32 DustCloud_Curves_84[12] = {
#include "game/008D36E0.inc"
};
u32 DustCloud_Curves_87[12] = {
#include "game/008D3710.inc"
};
u32 DustCloud_Curves_90[12] = {
#include "game/008D3740.inc"
};
u32 DustCloud_Curves_93[12] = {
#include "game/008D3770.inc"
};
u32 DustCloud_Curves_73[8] = {
#include "game/008D37A0.inc"
};
u32 DustCloud_Curves_76[8] = {
#include "game/008D37C0.inc"
};
u32 DustCloud_Curves_82[8] = {
#include "game/008D37E0.inc"
};
u32 DustCloud_Curves_88[8] = {
#include "game/008D3800.inc"
};
s16 DustCloud_DelayLists_9[8] = {
#include "game/008D3820.inc"
};
u16 DustCloud_DurLists_9[8] = {
#include "game/008D3830.inc"
};
u32 DustCloud_Curves_95[24] = {
#include "game/008D3840.inc"
};
u32 DustCloud_Curves_98[20] = {
#include "game/008D38A0.inc"
};
u32 DustCloud_Curves_96[16] = {
#include "game/008D38F0.inc"
};
u32 DustCloud_Curves_99[20] = {
#include "game/008D3930.inc"
};
u32 DustCloud_Curves_97[12] = {
#include "game/008D3980.inc"
};
u32 DustCloud_Curves_100[8] = {
#include "game/008D39B0.inc"
};
u16 DustCloud_DurLists_10[8] = {
#include "game/008D39D0.inc"
};
u32 DustCloud_Curves_101[20] = {
#include "game/008D39E0.inc"
};
u32 DustCloud_Curves_102[20] = {
#include "game/008D3A30.inc"
};
u32 DustCloud_Curves_103[10] = {
#include "game/008D3A80.inc"
};
s16 DustCloud_DelayLists_11[6] = {
#include "game/008D3AA8.inc"
};
u32 DustCloud_Curves_104[16] = {
#include "game/008D3AC0.inc"
};
u32 DustCloud_Curves_105[16] = {
#include "game/008D3B00.inc"
};
u32 DustCloud_Curves_106[8] = {
#include "game/008D3B40.inc"
};
u32 DustCloud_Curves_107[12] = {
#include "game/008D3B60.inc"
};
u32 DustCloud_Curves_108[12] = {
#include "game/008D3B90.inc"
};
u8 DustCloud_FlatPuffColors[32] = {
#include "game/008D3BC0.inc"
};
u32 DustCloud_Curves_109[20] = {
#include "game/008D3BE0.inc"
};
u32 DustCloud_Curves_112[20] = {
#include "game/008D3C30.inc"
};
u32 DustCloud_Curves_110[16] = {
#include "game/008D3C80.inc"
};
u32 DustCloud_Curves_113[12] = {
#include "game/008D3CC0.inc"
};
u32 DustCloud_Curves_111[8] = {
#include "game/008D3CF0.inc"
};
u32 DustCloud_Curves_114[8] = {
#include "game/008D3D10.inc"
};
s16 DustCloud_DelayLists_14[4] = {
#include "game/008D3D30.inc"
};
u16 DustCloud_DurLists_14[4] = {
#include "game/008D3D38.inc"
};
u32 DustCloud_Curves_115[28] = {
#include "game/008D3D40.inc"
};
u32 DustCloud_Curves_118[28] = {
#include "game/008D3DB0.inc"
};
u32 DustCloud_Curves_121[28] = {
#include "game/008D3E20.inc"
};
u32 DustCloud_Curves_116[28] = {
#include "game/008D3E90.inc"
};
u32 DustCloud_Curves_119[28] = {
#include "game/008D3F00.inc"
};
u32 DustCloud_Curves_122[28] = {
#include "game/008D3F70.inc"
};
u32 DustCloud_Curves_117[8] = {
#include "game/008D3FE0.inc"
};
u32 DustCloud_Curves_120[8] = {
#include "game/008D4000.inc"
};
u32 DustCloud_Curves_123[6] = {
#include "game/008D4020.inc"
};
s16 DustCloud_DelayLists_15[4] = {
#include "game/008D4038.inc"
};
u16 DustCloud_DurLists_15[8] = {
#include "game/008D4040.inc"
};
u32 DustCloud_Curves_124[12] = {
#include "game/008D4050.inc"
};
u32 DustCloud_Curves_125[12] = {
#include "game/008D4080.inc"
};
u32 DustCloud_Curves_126[24] = {
#include "game/008D40B0.inc"
};
u32 DustCloud_Curves_127[16] = {
#include "game/008D4110.inc"
};
u32 DustCloud_Curves_128[12] = {
#include "game/008D4150.inc"
};
f32 DustCloud_SideOffsets[4] = {
#include "game/008D4180.inc"
};
u32 DustCloud_Curves_129[28] = {
#include "game/008D4190.inc"
};
u32 DustCloud_Curves_132[20] = {
#include "game/008D4200.inc"
};
u32 DustCloud_Curves_135[28] = {
#include "game/008D4250.inc"
};
u32 DustCloud_Curves_130[20] = {
#include "game/008D42C0.inc"
};
u32 DustCloud_Curves_133[20] = {
#include "game/008D4310.inc"
};
u32 DustCloud_Curves_136[20] = {
#include "game/008D4360.inc"
};
u32 DustCloud_Curves_131[8] = {
#include "game/008D43B0.inc"
};
u32 DustCloud_Curves_134[8] = {
#include "game/008D43D0.inc"
};
u32 DustCloud_Curves_137[8] = {
#include "game/008D43F0.inc"
};
u16 DustCloud_DurLists_19[8] = {
#include "game/008D4410.inc"
};
u32 DustCloud_Curves_138[24] = {
#include "game/008D4420.inc"
};
u32 DustCloud_Curves_141[16] = {
#include "game/008D4480.inc"
};
u32 DustCloud_Curves_139[28] = {
#include "game/008D44C0.inc"
};
u32 DustCloud_Curves_142[12] = {
#include "game/008D4530.inc"
};
u32 DustCloud_Curves_140[12] = {
#include "game/008D4560.inc"
};
u32 DustCloud_Curves_143[8] = {
#include "game/008D4590.inc"
};
s16 DustCloud_DelayLists_20[4] = {
#include "game/008D45B0.inc"
};
u16 DustCloud_DurLists_20[4] = {
#include "game/008D45B8.inc"
};
f32 DustCloud_PuffSizes[4] = {
#include "game/008D45C0.inc"
};
u32 DustCloud_Curves_150[16] = {
#include "game/008D45D0.inc"
};
u32 DustCloud_Curves_151[16] = {
#include "game/008D4610.inc"
};
u8 DustCloud_RingColors[48] = {
#include "game/008D4650.inc"
};
u8 DustCloud_RingColorsAlt[48] = {
#include "game/008D4680.inc"
};
u32 DustCloud_Curves_152[20] = {
#include "game/008D46B0.inc"
};
u32 DustCloud_Curves_154[20] = {
#include "game/008D4700.inc"
};
u32 DustCloud_Curves_153[12] = {
#include "game/008D4750.inc"
};
u32 DustCloud_Curves_155[12] = {
#include "game/008D4780.inc"
};
u32 DustCloud_Curves_157[12] = {
#include "game/008D47B0.inc"
};
u32 DustCloud_Curves_159[12] = {
#include "game/008D47E0.inc"
};
u32 DustCloud_Curves_161[12] = {
#include "game/008D4810.inc"
};
u32 DustCloud_Curves_163[12] = {
#include "game/008D4840.inc"
};
u32 DustCloud_Curves_165[12] = {
#include "game/008D4870.inc"
};
u32 DustCloud_Curves_167[12] = {
#include "game/008D48A0.inc"
};
u8 DustCloud_BreathRathalosCore[32] = {
#include "game/008D48D0.inc"
};
u8 DustCloud_BreathRathalosTrail[32] = {
#include "game/008D48F0.inc"
};
u8 DustCloud_BreathGypcerosCore[32] = {
#include "game/008D4910.inc"
};
u8 DustCloud_BreathGypcerosTrail[32] = {
#include "game/008D4930.inc"
};
u8 DustCloud_BreathKutKuCore[32] = {
#include "game/008D4950.inc"
};
u8 DustCloud_BreathKutKuTrail[32] = {
#include "game/008D4970.inc"
};
u8 DustCloud_BreathKhezuCore[32] = {
#include "game/008D4990.inc"
};
u8 DustCloud_BreathKhezuTrail[32] = {
#include "game/008D49B0.inc"
};
u8 DustCloud_BreathGraviosCore[32] = {
#include "game/008D49D0.inc"
};
u8 DustCloud_BreathGraviosTrail[32] = {
#include "game/008D49F0.inc"
};
u8 DustCloud_BreathBasariosCore[32] = {
#include "game/008D4A10.inc"
};
u8 DustCloud_BreathBasariosTrail[32] = {
#include "game/008D4A30.inc"
};
u8 DustCloud_BreathOtherCore[32] = {
#include "game/008D4A50.inc"
};
u8 DustCloud_BreathOtherTrail[32] = {
#include "game/008D4A70.inc"
};
u16 DustCloud_DurLists_25[8] = {
#include "game/008D4A90.inc"
};
u32 DustCloud_Curves_168[16] = {
#include "game/008D4AA0.inc"
};
u8 DustCloud_PuffRathalosFlame[32] = {
#include "game/008D4AE0.inc"
};
u8 DustCloud_PuffRathalosSmoke[32] = {
#include "game/008D4B00.inc"
};
u8 DustCloud_PuffGypceros[32] = {
#include "game/008D4B20.inc"
};
u8 DustCloud_PuffKutKuFlame[32] = {
#include "game/008D4B40.inc"
};
u8 DustCloud_PuffKutKuSmoke[32] = {
#include "game/008D4B60.inc"
};
u8 DustCloud_PuffKhezu[32] = {
#include "game/008D4B80.inc"
};
u8 DustCloud_PuffPlesioth[32] = {
#include "game/008D4BA0.inc"
};
u8 DustCloud_PuffCephadrome[32] = {
#include "game/008D4BC0.inc"
};
u8 DustCloud_PuffGravios[32] = {
#include "game/008D4BE0.inc"
};
u8 DustCloud_PuffBasarios[32] = {
#include "game/008D4C00.inc"
};
u8 DustCloud_PuffOther[32] = {
#include "game/008D4C20.inc"
};
u8 DustCloud_PuffFatalisFlame[32] = {
#include "game/008D4C40.inc"
};
u8 DustCloud_PuffFatalisSmoke[32] = {
#include "game/008D4C60.inc"
};
u32 DustCloud_Curves_169[24] = {
#include "game/008D4C80.inc"
};
u32 DustCloud_Curves_170[16] = {
#include "game/008D4CE0.inc"
};
u32 DustCloud_Curves_171[8] = {
#include "game/008D4D20.inc"
};
u32 DustCloud_Curves_172[20] = {
#include "game/008D4D40.inc"
};
u32 DustCloud_Curves_173[20] = {
#include "game/008D4D90.inc"
};
u8 DustCloud_ColorKeysByVar_0[48] = {
#include "game/008D4DE0.inc"
};
u8 DustCloud_ColorKeysByVar_1[48] = {
#include "game/008D4E10.inc"
};
u8 DustCloud_ColorKeysByVar_2[40] = {
#include "game/008D4E40.inc"
};
u8 *DustCloud_SectorColors[3] = {
    DustCloud_ColorKeysByVar_0, DustCloud_ColorKeysByVar_1, DustCloud_ColorKeysByVar_2,
};
s16 DustCloud_PuffSectors[6] = {
#include "game/008D4E78.inc"
};
void *DustCloud_Curves[176] = {
    (void *)DustCloud_Curves_0, (void *)DustCloud_Curves_1, (void *)DustCloud_Curves_2, (void *)DustCloud_Curves_3,
    (void *)DustCloud_Curves_4, (void *)DustCloud_Curves_5, (void *)DustCloud_Curves_6, (void *)DustCloud_Curves_7,
    (void *)DustCloud_Curves_8, (void *)DustCloud_Curves_9, (void *)DustCloud_Curves_10, (void *)DustCloud_Curves_11,
    (void *)DustCloud_Curves_12, (void *)DustCloud_Curves_13, (void *)DustCloud_Curves_14, (void *)DustCloud_Curves_15,
    (void *)DustCloud_Curves_16, (void *)DustCloud_Curves_17, (void *)DustCloud_Curves_18, (void *)DustCloud_Curves_19,
    (void *)DustCloud_Curves_20, (void *)DustCloud_Curves_21, (void *)DustCloud_Curves_22, (void *)DustCloud_Curves_23,
    (void *)DustCloud_Curves_24, (void *)DustCloud_Curves_25, (void *)DustCloud_Curves_26, (void *)DustCloud_Curves_27,
    (void *)DustCloud_Curves_28, (void *)DustCloud_Curves_29, (void *)DustCloud_Curves_30, (void *)DustCloud_Curves_31,
    (void *)DustCloud_Curves_32, (void *)DustCloud_Curves_33, (void *)DustCloud_Curves_34, (void *)DustCloud_Curves_35,
    (void *)DustCloud_Curves_36, (void *)DustCloud_Curves_37, (void *)DustCloud_Curves_38, (void *)DustCloud_Curves_39,
    (void *)DustCloud_Curves_40, (void *)DustCloud_Curves_41, (void *)DustCloud_Curves_42, (void *)DustCloud_Curves_43,
    (void *)DustCloud_Curves_44, (void *)DustCloud_Curves_45, (void *)DustCloud_Curves_46, (void *)DustCloud_Curves_47,
    (void *)DustCloud_Curves_48, (void *)DustCloud_Curves_49, (void *)DustCloud_Curves_50, (void *)DustCloud_Curves_51,
    (void *)DustCloud_Curves_52, (void *)DustCloud_Curves_53, (void *)DustCloud_Curves_54, (void *)DustCloud_Curves_55,
    (void *)DustCloud_Curves_56, (void *)DustCloud_Curves_57, (void *)DustCloud_Curves_58, (void *)DustCloud_Curves_59,
    (void *)DustCloud_Curves_60, (void *)DustCloud_Curves_61, (void *)DustCloud_Curves_62, (void *)DustCloud_Curves_63,
    (void *)DustCloud_Curves_64, (void *)DustCloud_Curves_65, (void *)DustCloud_Curves_66, (void *)DustCloud_Curves_67,
    (void *)DustCloud_Curves_68, (void *)DustCloud_Curves_69, (void *)DustCloud_Curves_70, (void *)DustCloud_Curves_71,
    (void *)DustCloud_Curves_72, (void *)DustCloud_Curves_73, (void *)DustCloud_Curves_74, (void *)DustCloud_Curves_75,
    (void *)DustCloud_Curves_76, (void *)DustCloud_Curves_74, (void *)DustCloud_Curves_78, (void *)DustCloud_Curves_76,
    (void *)DustCloud_Curves_80, (void *)DustCloud_Curves_81, (void *)DustCloud_Curves_82, (void *)DustCloud_Curves_80,
    (void *)DustCloud_Curves_84, (void *)DustCloud_Curves_82, (void *)DustCloud_Curves_86, (void *)DustCloud_Curves_87,
    (void *)DustCloud_Curves_88, (void *)DustCloud_Curves_80, (void *)DustCloud_Curves_90, (void *)DustCloud_Curves_82,
    (void *)DustCloud_Curves_80, (void *)DustCloud_Curves_93, (void *)DustCloud_Curves_82, (void *)DustCloud_Curves_95,
    (void *)DustCloud_Curves_96, (void *)DustCloud_Curves_97, (void *)DustCloud_Curves_98, (void *)DustCloud_Curves_99,
    (void *)DustCloud_Curves_100, (void *)DustCloud_Curves_101, (void *)DustCloud_Curves_102, (void *)DustCloud_Curves_103,
    (void *)DustCloud_Curves_104, (void *)DustCloud_Curves_105, (void *)DustCloud_Curves_106, (void *)DustCloud_Curves_107,
    (void *)DustCloud_Curves_108, (void *)DustCloud_Curves_109, (void *)DustCloud_Curves_110, (void *)DustCloud_Curves_111,
    (void *)DustCloud_Curves_112, (void *)DustCloud_Curves_113, (void *)DustCloud_Curves_114, (void *)DustCloud_Curves_115,
    (void *)DustCloud_Curves_116, (void *)DustCloud_Curves_117, (void *)DustCloud_Curves_118, (void *)DustCloud_Curves_119,
    (void *)DustCloud_Curves_120, (void *)DustCloud_Curves_121, (void *)DustCloud_Curves_122, (void *)DustCloud_Curves_123,
    (void *)DustCloud_Curves_124, (void *)DustCloud_Curves_125, (void *)DustCloud_Curves_126, (void *)DustCloud_Curves_127,
    (void *)DustCloud_Curves_128, (void *)DustCloud_Curves_129, (void *)DustCloud_Curves_130, (void *)DustCloud_Curves_131,
    (void *)DustCloud_Curves_132, (void *)DustCloud_Curves_133, (void *)DustCloud_Curves_134, (void *)DustCloud_Curves_135,
    (void *)DustCloud_Curves_136, (void *)DustCloud_Curves_137, (void *)DustCloud_Curves_138, (void *)DustCloud_Curves_139,
    (void *)DustCloud_Curves_140, (void *)DustCloud_Curves_141, (void *)DustCloud_Curves_142, (void *)DustCloud_Curves_143,
    (void *)DustCloud_Curves_141, (void *)DustCloud_Curves_142, (void *)DustCloud_Curves_143, (void *)DustCloud_Curves_141,
    (void *)DustCloud_Curves_142, (void *)DustCloud_Curves_143, (void *)DustCloud_Curves_150, (void *)DustCloud_Curves_151,
    (void *)DustCloud_Curves_152, (void *)DustCloud_Curves_153, (void *)DustCloud_Curves_154, (void *)DustCloud_Curves_155,
    (void *)DustCloud_Curves_152, (void *)DustCloud_Curves_157, (void *)DustCloud_Curves_154, (void *)DustCloud_Curves_159,
    (void *)DustCloud_Curves_152, (void *)DustCloud_Curves_161, (void *)DustCloud_Curves_154, (void *)DustCloud_Curves_163,
    (void *)DustCloud_Curves_152, (void *)DustCloud_Curves_165, (void *)DustCloud_Curves_154, (void *)DustCloud_Curves_167,
    (void *)DustCloud_Curves_168, (void *)DustCloud_Curves_169, (void *)DustCloud_Curves_170, (void *)DustCloud_Curves_171,
    (void *)DustCloud_Curves_172, (void *)DustCloud_Curves_173, NULL, NULL,
};
s16 DustCloud_PartCounts[32] = {
#include "game/008D5150.inc"
};
u16 DustCloud_Lifetimes[32] = {
#include "game/008D5190.inc"
};
s16 *DustCloud_StartTimes[32] = {
    NULL, DustCloud_DelayLists_1, DustCloud_DelayLists_2, DustCloud_DelayLists_2,
    DustCloud_DelayLists_4, (s16 *)&DustCloud_Curves_44[10], DustCloud_DelayLists_6, NULL,
    DustCloud_DelayLists_8, DustCloud_DelayLists_9, NULL, DustCloud_DelayLists_11,
    NULL, NULL, DustCloud_DelayLists_14, DustCloud_DelayLists_15,
    NULL, NULL, NULL, NULL,
    DustCloud_DelayLists_20, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u16 *DustCloud_PuffDurations[32] = {
    NULL, NULL, DustCloud_DurLists_2, DustCloud_DurLists_2,
    DustCloud_DurLists_4, (u16 *)&DustCloud_Curves_44[14], DustCloud_DurLists_6, NULL,
    DustCloud_DurLists_8, DustCloud_DurLists_9, DustCloud_DurLists_10, NULL,
    NULL, NULL, DustCloud_DurLists_14, DustCloud_DurLists_15,
    NULL, NULL, NULL, DustCloud_DurLists_19,
    DustCloud_DurLists_20, NULL, NULL, NULL,
    NULL, DustCloud_DurLists_25, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
s16 DustCloud_CurveIdx[32] = {
#include "game/008D52D0.inc"
};
s16 DustCloud_CurvesPerPuff[32] = {
#include "game/008D5310.inc"
};
s16 DustCloud_SurfaceFx[32] = {
#include "game/008D5350.inc"
};
u8 MashDebris_PartCounts[8] = {
#include "game/008D5390.inc"
};
u8 MashDebris_Models[8] = {
#include "game/008D5398.inc"
};
u16 MashDebris_Lives[8] = {
#include "game/008D53A0.inc"
};
