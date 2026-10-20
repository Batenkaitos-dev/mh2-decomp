/*
 * The stage objects of kinds 0x1B, 0x1C, 0x1D and 0x1F: MashDebris (the
 * pieces a player sheds in the
 *
 * mash status), MonsterFx (the visual effects of the monsters' moves and
 * attacks), AreaFlyer (a small
 *
 * thing flying around a point of some areas) and the set-up and run of GunFx
 * (the gun weapons' flashes,
 *
 * smoke and spent shells, and a Hermitaur's spray). Each kind's update, run
 * every frame from the stage
 *
 * object list, switches on StageObj.state to its Setup, Run, End and Free;
 * the Run puts each part's
 *
 * DrawSlot in a draw table and the kind's Draw renders it. An object keeps
 * its parts in its blocks
 *
 * (StageObj.buf): MashDebrisPiece; MonsterFxLink for the MonsterFx chains
 * (types 0 and 0xB), which
 *
 * trail behind a bone; MonsterFxPiece for the other MonsterFx types, each
 * part playing the steps of
 *
 * its type's tables (step lengths, models, scale curves, colour keys);
 * GunFxWork for GunFx. MashDebris's
 *
 * update and set-up, and GunFx's end, draw and creators, are defined with the
 * other stage objects.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "area.h"
/* Declarations of the functions called below. */
void DrawSlot_Free(s32 i);
/* Player_InCurrentArea returns a u8; the callers that mask it again (a u8 cast or & 0xFF) cast it
 * to s32 first, so the compiler keeps the mask the game's code has. */
u8 Player_InCurrentArea(void *obj);
s32 MonsterFx_OwnerActive(Em *em);

/* Built at optimization level 4, as the game's code was; MonsterFx_NextDustKickStep alone is built
 * at level 3. */
#pragma optimization_level 4

/* a link of a MonsterFx chain (types 0 and 0xB): 0x64 bytes each, in the object's blocks */
typedef struct MonsterFxLink {
    s16 slot;                   /* 0x00: its draw slot */
    s16 kind;                   /* 0x02: 0 the first part, 1 the second, others follow */
    f32 pos[3];                 /* 0x04 */
    s16 frame;                  /* 0x10: 8 - its index; a frame (texture scroll) */
    s16 releaseFrame;                  /* 0x12: -1 - its index */
    u16 tiltX;                  /* 0x14: extra angles of the other parts (type 0xB) */
    u16 tiltY;                  /* 0x16 */
    u16 rotZ;                  /* 0x18: its z angle (random at set-up and at release: MonsterFx_RandomSpin) */
    s16 spinZ;                  /* 0x1A: its speed: 0xB6 for the first part; -0x100..0xFF */
    f32 scale;                  /* 0x1C: the object's, + 0.2 per part */
    DrawSlot *node;                /* 0x20 */
    f32 sc[3];                  /* 0x24: a scale from a curve (1.0 at first) */
    s32 color;                  /* 0x30: from a curve, -1 at first */
    f32 base[2][3];             /* 0x34: the last two base points */
    f32 dir[2][3];              /* 0x4C: the last two directions */
} MonsterFxLink;

/* a part of a MonsterFx object of the other types (0x40 bytes each): it plays the steps of its
 * type's tables */
typedef struct MonsterFxPiece {
    s16 slot;                   /* 0x00: its draw slot */
    u8 idx;                     /* 0x02: the entry of the step table, 0xFF: done */
    u8 sub;                     /* 0x03 */
    f32 pos[3];                 /* 0x04 */
    u32 color;                  /* 0x10 */
    f32 sc[3];                  /* 0x14: its scale from its scale curve (x, y and, unless flatScale, z) */
    f32 scale;                  /* 0x20 */
    DrawSlot *node;                /* 0x24 */
    s16 wait;                   /* 0x28: its frame in the current step (negative: frames still to wait) */
    u16 rotX;                  /* 0x2A: angles (x, y, z) */
    u16 rotY;                  /* 0x2C */
    u16 rotZ;                  /* 0x2E */
    s16 spinX;                  /* 0x30: their speeds */
    s16 spinY;                  /* 0x32 */
    s16 spinZ;                  /* 0x34 */
    u8 useColorKeys;                   /* 0x36 */
    u8 flatScale;                   /* 0x37 */
    f32 speed;                  /* 0x38 */
    f32 riseSpeed;                  /* 0x3C */
} MonsterFxPiece;


/* prototypes */
extern u16 MonsterFx_TescaAuraFrames[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
u16 Vec_Elevation(f32 *a, f32 *b);
u16 Vec_Heading(f32 *a, f32 *b);
void MonsterFx_RandomSpin(MonsterFxLink *w);
extern f32 *gStageObjK1C_ScaleCurves[];
extern u8 *gStageObjK1C_ColorKeys[];
extern s16 gStageObjK1C_ScaleCurveIdx[];
extern s16 gStageObjK1C_ColorCurveIdx[];
s32 Em_PlaySeChecked(struct Em *a0, s32 id, s32 a, void *a3, s32 b, s32 c);
extern void *MashDebris_ScaleKeys[];
extern void *MashDebris_AlphaKeys[];
extern f32 MashDebris_SpinDeg[];
extern u8 MashDebris_PartFalls[];
extern u8 MashDebris_TrickleScaleKeys[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void MonsterFx_FollowBone(StageObj *o, MonsterFxLink *w, MonsterFxLink *prev, f32 *m, f32 *dir, s16 first);
void MonsterFx_Draw(DrawSlot *node);
s32 Effect_DrawModelAdd(void *arg0, f32 *m, void *mats);
s32 EmHermitaur_SprayBoosted(Em *em);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Vec_RotateX(f32 *v, f32 a);
void Vec_RotateY(f32 *v, f32 a);
f32 MonsterFx_GrowFactor(StageObj *o);
extern s16 MonsterFx_DustBurstFrames[];
extern u16 MonsterFx_KushalaRingFrames[];
f32 Rand_Unit(void);
extern u8 D_008D6738[];
extern u16 MonsterFx_BreathDustStarts[];
extern u8 MonsterFx_ImpactDustKinds[];
extern u16 MonsterFx_ImpactDustStarts[];
extern u8 MonsterFx_DustKickKinds[];
extern u16 MonsterFx_DustKickStarts[];
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gAreaFlyerScaleKeys[];
extern u8 gAreaFlyerColorKeys[];
extern f32 gCamMatrix[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *n);
void Mtx_Identity(f32 *m);
void Vec_Normalize(f32 *v);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
void Key_LerpVec2(f32 *a, f32 *b, f32 f);
void Color_EvalKeys(s32 *a, s32 t, u32 *c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void AreaFlyer_Draw(DrawSlot *node);
u16 Rand_Next(s32 stream);
void MonsterFx_Setup(StageObj *o);
void MonsterFx_Run(StageObj *o);
void MonsterFx_End(StageObj *o);
void MonsterFx_Free(StageObj *o);
void AreaFlyer_Setup(StageObj *o);
void AreaFlyer_Run(StageObj *o);
void AreaFlyer_End(StageObj *o);
void AreaFlyer_Free(StageObj *o);
void AreaFlyer_GetCenter(f32 *out, u8 idx);
void GunFx_Setup(StageObj *o);
void GunFx_Run(StageObj *o);
void GunFx_End(StageObj *o);
void GunFx_Free(StageObj *o);
void Vec_Copy(f32 *out, f32 *in);
extern f32 gAreaFlyerCenters[][3];
extern u8 MonsterFx_TescaAuraKinds[];
extern u16 MonsterFx_TescaAuraStarts[];
f32 Rand_Signed(void);
extern u8 D_008D6138[];
extern u16 MonsterFx_KushalaRingStarts[];
void MashDebris_Update(StageObj *o);
StageObj *MonsterFx_Alloc(u8 k);
extern s16 gStageObjK1C_Counts[];
void MonsterFx_EndChain(StageObj *o, s16 n);
void MonsterFx_EndSeq(StageObj *o, s16 n);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern u8 *gStageObjK1C_ModelSets[];
s16 EmSlot_FindKind(s16 k);
extern u8 MashDebris_PartCounts[];
void MonsterFx_SetupChain(StageObj *o);
void MonsterFx_SetupSeq(StageObj *o);
void MonsterFx_DrawChain(DrawSlot *node);
void MonsterFx_DrawSeq(DrawSlot *node);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void MonsterFx_RunChain(StageObj *o);
void MonsterFx_RunSeq(StageObj *o);
extern s16 gStageObjK1C_Limits[];
/* end of prototypes */

/*
 * Runs a MashDebris object each frame. With no owner it ends at once (state
 * 2); it also ends, hidden, when every piece has lived its frames (life). A
 * burst (type 0) fades its pieces by their alpha keys, sizes them by their
 * scale keys, spins them (var0 picks the way) and moves them by their
 * velocity and fall; pieces 0 and 4 end when they drop 20 below their floor.
 * A trickle piece (type 1) moves to the owner's bone 2 on its first frame,
 * then falls faster and faster with MashDebris_TrickleScaleKeys.
 */
void MashDebris_Run(StageObj *obj)
{
    f32 alphaKey;
    f32 bonePos[3];
    MashDebrisPiece *piece = (MashDebrisPiece *) obj->buf;
    u8 done = 0;
    u8 i;

    if (obj->owner == NULL) {
        obj->state++;
        return;
    }
    for (i = 0; i < MashDebris_PartCounts[obj->type]; i++) {
        if (++piece->t >= piece->life) {
            piece++;
            if (++done == MashDebris_PartCounts[obj->type]) {
                obj->state++;
                obj->visible = 0;
                return;
            }
            continue;
        }
        switch (obj->type) {
        case 0:
            if (i == 0 || i == 4) {
                if (piece->pos[1] < piece->ground - 20.0f) {
                    piece->life = 0;
                }
            }
            if (MashDebris_ScaleKeys[i] != NULL) {
                Key_LerpVec3(MashDebris_ScaleKeys[i], piece->scale, piece->t);
            }
            Key_LerpFloat(MashDebris_AlphaKeys[i], &alphaKey, piece->t);
            piece->alpha = 255.0f * alphaKey;
            if (obj->var0 == 0) {
                piece->rot[2] += (u16) (s32) (0.5f + (65536.0f * MashDebris_SpinDeg[i]) / 360.0f);
            } else {
                piece->rot[2] -= (u16) (s32) (0.5f + (65536.0f * MashDebris_SpinDeg[i]) / 360.0f);
            }
            if (MashDebris_PartFalls[i] != 0) {
                piece->pos[1] -= piece->fall;
                if (piece->fall < 20.0f) {
                    piece->fall += 2.0f;
                }
            }
            piece->pos[1] -= piece->fall;
            piece->pos[0] += piece->vel[0];
            piece->pos[1] += piece->vel[1];
            piece->pos[2] += piece->vel[2];
            break;
        case 1:
            if (piece->t > 0) {
                if (piece->t == 1) {
                    Em_GetBonePos(obj->owner, 2, bonePos);
                    piece->pos[0] += bonePos[0];
                    piece->pos[1] += bonePos[1] - 50.0f;
                    piece->pos[2] += bonePos[2];
                }
                piece->pos[1] -= piece->fall;
                if (piece->fall < 8.0f) {
                    piece->fall += 0.5f;
                }
                Key_LerpVec3((f32 *)MashDebris_TrickleScaleKeys, piece->scale, piece->t);
            }
            break;
        }
        if (piece->node != NULL) {
            piece->node->pos[0] = piece->pos[0];
            piece->node->pos[1] = piece->pos[1];
            piece->node->pos[2] = piece->pos[2];
            if (obj->type == 0 && i == 0) {
                OT_AddByDepth(gDrawOt32, piece->node, 0x20, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
            }
        }
        piece++;
    }
}

/*
 * Frees the draw slots of the object's pieces and moves it to its free state.
 */
void MashDebris_End(StageObj *obj)
{
    MashDebrisPiece *piece = (MashDebrisPiece *) obj->buf;
    u8 i;

    obj->state++;
    for (i = 0; i < MashDebris_PartCounts[obj->type]; i++) {
        if (piece->node != NULL) {
            DrawSlot_Free(piece->slot);
        }
        piece++;
    }
}

/* Frees the object (StageObj_Free). */
void MashDebris_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws one piece (arg is its DrawSlot) from the stage's effect model set,
 * scaled, rotated by the piece's angles and turned to face the camera, in the
 * piece's alpha. The burst's pieces 0 and 4 do not face the camera and are
 * drawn by Effect_DrawModelAdd in white (RS_COLOR2). Nothing is drawn while
 * the set is not loaded or the object is hidden.
 */
void MashDebris_Draw(void *slotArg)
{
    ModelMat *mats;
    ModelMatSet *model;
    u8 faceCamera = 1;
    s32 color;
    DrawSlot *node = slotArg;
    StageObj *obj = node->owner;
    Model *modelSet = gStageModelSet;
    MashDebrisPiece *piece = (MashDebrisPiece *) obj->buf + node->part;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];

    if (modelSet != NULL && modelSet->active != 0 && obj->visible != 0) {
        color = (piece->alpha << 24) | 0xFFFFFF;
        mats = modelSet->mats;
        model = &modelSet->matSets[piece->model];
        scale[0] = piece->scale[0];
        scale[1] = piece->scale[1];
        scale[2] = piece->scale[2];
        rot[0] = (6.2831855f * piece->rot[0]) / 65536.0f;
        rot[1] = (6.2831855f * piece->rot[1]) / 65536.0f;
        rot[2] = (6.2831855f * piece->rot[2]) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 7, mtx);
        switch (obj->type) {
        case 0:
            if (node->part == 0 || node->part == 4) {
                faceCamera = 0;
            }
            break;
        }
        if (faceCamera) {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        if (obj->type == 0 && (node->part == 0 || node->part == 4)) {
            Gs_SetState(0x67, -1);
            Effect_DrawModelAdd(model, mtx, mats);
        } else {
            Effect_DrawModel(model, mtx, color, 0, mats);
        }
    }
}

s32 Obj_InCurrentArea(void *obj);
/*
 * Creates a MashDebris object of type k (0: the burst, 1: a trickle piece) on
 * the player arg, when he is in the current area.
 */
void MashDebris_Create(s8 variant, s16 *player)
{
    StageObj *obj;

    if (Obj_InCurrentArea(player) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_MASH_DEBRIS;
            obj->update = MashDebris_Update;
            obj->type = variant;
            obj->owner = (void *) player;
        }
    }
}

/*
 * The MonsterFx update: switches on the state to MonsterFx_Setup,
 * MonsterFx_Run, MonsterFx_End and MonsterFx_Free.
 */
void MonsterFx_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        MonsterFx_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        MonsterFx_Run(obj);
        break;
    case STAGEOBJ_END:
        MonsterFx_End(obj);
        break;
    case STAGEOBJ_FREE:
        MonsterFx_Free(obj);
        break;
    }
}

/*
 * Sets a new MonsterFx object up (visible, timer 0) and moves it to its run
 * state: the chain types (MFX_BREATH_STREAM, MFX_SPARE_CHAIN) through
 * MonsterFx_SetupChain, the others through MonsterFx_SetupSeq.
 */
void MonsterFx_Setup(StageObj *obj)
{
    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    switch (obj->type) {
    case 0:
    case 0xB:
        MonsterFx_SetupChain(obj);
        break;
    default:
        MonsterFx_SetupSeq(obj);
        break;
    }
}

/*
 * Sets up the links of a breath stream: each link (gMonsterFx_PartCounts of
 * them, each 0.2 larger than the one before) starts at the owner's breath
 * bone (MonsterFx_FollowBone) with its frame, its release counter, a random z
 * angle and a draw slot. Any other type frees the object at once.
 */
void MonsterFx_SetupChain(StageObj *obj)
{
    MonsterFxLink *link = (MonsterFxLink *) obj->buf;
    Em *em = obj->owner;
    MonsterFxLink *prev;
    MonsterFxLink *first;
    f32 *boneMtx;
    s16 i;
    s16 linkCount;

    linkCount = gStageObjK1C_Counts[obj->type];

    switch (obj->type) {
    case 0:
        boneMtx = (f32 *)Em_Bone(em, 0x25);
        first = link;
        break;
    default:
        MonsterFx_Free(obj);
        return;
    }
    for (i = 0; i < linkCount; i++, link++) {
        link->frame = 0;
        link->kind = i;
        link->scale = obj->scale;
        switch (obj->type) {
        case 0:
            MonsterFx_FollowBone(obj, link, prev, boneMtx, first->dir[0], 0);
            link->frame = 8 - i;
            link->releaseFrame = -1 - i;
            link->rotZ = Rand_Next(1);
            if (link->kind == 0) {
                link->spinZ = 0xB6;
            } else {
                link->spinZ = 0;
            }
            link->scale += 0.2f * i;
            link->sc[0] = 1.0f;
            link->sc[1] = 1.0f;
            link->sc[2] = 1.0f;
            link->color = -1;
            prev = link;
            break;
        }
        link->slot = DrawSlot_Alloc();
        if (link->slot != -1) {
            link->node = DrawSlot_Get(link->slot);
            link->node->owner = obj;
            link->node->part = i;
            link->node->draw = MonsterFx_Draw;
        } else {
            link->node = NULL;
        }
    }
}

extern u32 MonsterFx_BlastDelays[], MonsterFx_BlastRises[], MonsterFx_BlastSizes[], MonsterFx_ShardsDelays[], MonsterFx_GroundSprayKinds[], MonsterFx_GroundSprayRises[];
extern u32 MonsterFx_StrikeDepths[], MonsterFx_KushalaGustDelays[], MonsterFx_RajangRageStartKinds[], MonsterFx_ShenGaorenPuffDelays[], MonsterFx_RangedSplashDelays[], MonsterFx_RangedSplashFrames[];
extern u32 MonsterFx_ChameleosImpactKinds[], D_008D7318[], MonsterFx_ChameleosImpactFrames[], MonsterFx_ChameleosGroundBurstFrames[], MonsterFx_ChameleosGroundBurstKinds[], MonsterFx_ChameleosGroundBurstDelays[];
extern u32 MonsterFx_FatalisFireballDelays[], MonsterFx_KushalaGroundWindFrames[];
void StageObj_RandomDistinct(s16 n, s16 count, s16 *out);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void MonsterFx_PlaceBehindOwner(StageObj *o);
void MonsterFx_GetOwnerBone(StageObj *o, f32 *out);
void MonsterFx_Jitter(MonsterFxPiece *w, f32 r);
void MonsterFx_NextDustBurstStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_NextDustKickStep(StageObj *o, MonsterFxPiece *w);

/*
 * Sets up a MonsterFx object of a non-chain type. It first places the object
 * by its type: most at a bone of the owner plus an offset, some facing the
 * owner's heading, some with the owner's sound. Then each part
 * (gMonsterFx_PartCounts) gets its step (idx), its start delay (a negative
 * wait), position, size, angles and spins, from the type's tables or at
 * random, and a draw slot. The step-driven types take their first step from
 * their Next function.
 */
void MonsterFx_SetupSeq(StageObj *obj)
{
    f32 point[3];
    f32 offset[3];
    s16 quarters[4];
    MonsterFxPiece *piece = (MonsterFxPiece *) obj->buf;
    Em *em = obj->owner;
    s16 partCount = gStageObjK1C_Counts[obj->type];
    f32 *boneMtx;
    s16 i;
    s16 quartersUsed;
    f32 sizeRand;
    f32 y;
    u16 randBits;

    switch (obj->type) {
    case 3:
        Mtx_GetTrans(obj->pos, Em_Bone(em, 0x25)->world);
        break;
    case 4:
        Vec_Copy(point, obj->pos);
        Vec_Copy(obj->pos, piece->pos);
        point[0] += (Rand_Next(1) & 0x1FF) - 0x100;
        point[1] += (Rand_Next(1) & 0x1FF) - 0x100;
        point[2] += (Rand_Next(1) & 0x1FF) - 0x100;
        break;
    case 5:
    case 6:
    case 14:
        randBits = Rand_Next(1);
        break;
    case 7:
        boneMtx = (f32 *)Em_Bone(em, 0x12);
        offset[0] = 0.0f;
        offset[1] = -70.0f;
        offset[2] = 42.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        point[0] += offset[0];
        point[1] += offset[1];
        point[2] += offset[2];
        randBits = Rand_Next(1);
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
            obj->var1 = 0;
            break;
        case EM_KIND_CONGALALA:
            obj->var1 = 1;
            break;
        }
        break;
    case 8:
        boneMtx = (f32 *)Em_Bone(em, 0x12);
        offset[0] = 0.0f;
        offset[1] = -60.0f;
        offset[2] = 45.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        obj->angle = em->rotY;
        break;
    case 9:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 330.0f * obj->scale;
        Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        break;
    case 10:
    case 18:
        boneMtx = (f32 *)Em_Bone(em, 0x25);
        offset[0] = 0.0f;
        offset[1] = -60.0f;
        offset[2] = 150.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        obj->angle += em->rotY;
        break;
    case 12:
        obj->var0 = 1;
        obj->var1 = 0;
        obj->var2 = 0;
        obj->ownerIndex = 0;
        obj->angle = em->rage;
        obj->scale = em->scaleX;
        break;
    case 13:
        switch (em->type) {
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        default:
            obj->var1 = 0;
            boneMtx = (f32 *)Em_Bone(em, 0x27);
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 100.0f;
            break;
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            obj->var1 = 1;
            boneMtx = (f32 *)Em_Bone(em, 0x27);
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 100.0f;
            break;
        case EM_KIND_CHAMELEOS:
            obj->var1 = 2;
            boneMtx = (f32 *)Em_Bone(em, 0x29);
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 30.0f;
            break;
        case EM_KIND_BULLDROME:
            obj->var1 = 3;
            boneMtx = (f32 *)Em_Bone(em, 0xB);
            offset[0] = 0.0f;
            offset[1] = -10.0f;
            offset[2] = 20.0f;
            break;
        }
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        point[0] += offset[0];
        point[1] += offset[1];
        point[2] += offset[2];
        randBits = Rand_Next(1);
        break;
    case 15:
    case 17:
        obj->var2 = 0;
        break;
    case 16:
        Mtx_GetTrans(obj->pos, Em_Bone(em, 1)->world);
        obj->pos[1] = em->floorY;
        obj->angle = em->rotY;
        randBits = Rand_Next(1);
        break;
    case 19:
        boneMtx = (f32 *)Em_Bone(em, 0x1C);
        offset[0] = 0.0f;
        offset[1] = -80.0f;
        offset[2] = 0.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        obj->angle = em->rotY;
        y = em->floorY;
        break;
    case 20:
        boneMtx = (f32 *)Em_Bone(em, 0x1C);
        offset[0] = 0.0f;
        offset[1] = -50.0f;
        offset[2] = 0.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        Em_PlaySeChecked((struct Em *)em, 0x25, 0, obj->pos, 6, 0);
        break;
    case 21:
        boneMtx = (f32 *)Em_Bone(em, 1);
        offset[0] = 0.0f;
        offset[1] = 100.0f;
        offset[2] = -30.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        Em_PlaySeChecked((struct Em *)em, 0x25, 0, obj->pos, 6, 0);
        Em_PlaySeChecked((struct Em *)em, 0x18, 0, obj->pos, 6, 0);
        break;
    case 23:
        boneMtx = (f32 *)Em_Bone(em, 0x11);
        offset[0] = 0.0f;
        offset[1] = -110.0f;
        offset[2] = 80.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        obj->angle = em->rotY;
        break;
    case 24:
        Em_BonePos(em, 4, obj->pos);
        obj->pos[1] -= 100.0f;
        obj->angle = em->rotY;
        randBits = Rand_Next(1);
        Em_PlaySeChecked((struct Em *)em, 0x3C, 0, obj->pos, 7, 0);
        break;
    case 25:
        MonsterFx_PlaceBehindOwner(obj);
        break;
    case 26:
        MonsterFx_PlaceBehindOwner(obj);
        break;
    case 28:
        boneMtx = (f32 *)Em_Bone(em, 0x1F);
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 100.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        obj->angle2 += (Rand_Next(1) & 0x3FF) - 0x200;
        obj->angle += (Rand_Next(1) & 0x3FF) - 0x200;
        break;
    case 29:
        obj->var0 = 0;
        obj->var1 = 0;
        obj->var2 = 0;
        obj->ownerIndex = 0;
        obj->scale = em->scaleX;
        obj->angle = 0;
        break;
    case 32:
    case 33:
        StageObj_RandomDistinct(4, 4, quarters);
        quartersUsed = 0;
        break;
    case 34:
        Vec_Copy(point, obj->pos);
        obj->pos[1] = em->floorY;
        break;
    case 37:
        boneMtx = (f32 *)Em_Bone(em, 0x13);
        offset[0] = 0.0f;
        offset[1] = -50.0f;
        offset[2] = 60.0f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        break;
    case 39:
        Em_BonePos(em, 0x10, obj->pos);
        randBits = Rand_Next(1);
        break;
    case 40:
        MonsterFx_GetOwnerBone(obj, point);
        if (obj->var2 == 0) {
            Em_PlaySeChecked((struct Em *)em, 0x36, 0, point, 8, 0);
        } else {
            Em_PlaySeChecked((struct Em *)em, 0x37, 0, point, 8, 0);
        }
        randBits = Rand_Next(1);
        break;
    }
    for (i = 0; i < partCount; i++, piece++) {
        piece->wait = 0;
        piece->idx = i;
        piece->sub = 0;
        piece->scale = obj->scale;
        piece->useColorKeys = 1;
        piece->flatScale = 1;
        piece->rotX = 0;
        piece->rotY = 0;
        piece->rotZ = 0;
        piece->spinX = 0;
        piece->spinY = 0;
        piece->spinZ = 0;
        switch (obj->type) {
        case 1:
            Vec_Copy(piece->pos, obj->pos);
            piece->wait = -((s16 *) MonsterFx_BlastDelays)[i];
            piece->pos[1] += ((f32 *) MonsterFx_BlastRises)[i];
            piece->scale *= ((f32 *) MonsterFx_BlastSizes)[i];
            switch (i) {
            case 0:
                piece->idx = 0;
                piece->rotZ = Rand_Next(1);
                piece->useColorKeys = 0;
                break;
            case 1:
                piece->idx = 1;
                piece->useColorKeys = 0;
                break;
            case 2:
            case 3:
                piece->idx = 2;
                piece->rotX = Rand_Next(1);
                piece->rotY = Rand_Next(1);
                piece->rotZ = Rand_Next(1);
                piece->spinX = (Rand_Next(1) & 0x1FF) - 0x100;
                piece->spinY = (Rand_Next(1) & 0x1FF) - 0x100;
                piece->spinZ = (Rand_Next(1) & 0x1FF) - 0x100;
                piece->flatScale = 0;
                break;
            case 4:
            case 5:
                piece->idx = 3;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = (Rand_Next(1) & 0x1FF) - 0x100;
                break;
            case 6:
            case 7:
            case 8:
                piece->idx = 4;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = (Rand_Next(1) & 0xFF) - 0x80;
                piece->useColorKeys = 1;
                piece->pos[0] += 75.0f * Rand_Signed();
                piece->pos[1] += 35.0f * Rand_Signed();
                piece->pos[2] += 25.0f * Rand_Signed();
                piece->useColorKeys = 0;
                break;
            }
            break;
        case 2:
            Vec_Copy(piece->pos, obj->pos);
            piece->wait = -((s16 *) MonsterFx_ShardsDelays)[i];
            switch (i) {
            case 0:
            case 1:
            case 2:
                piece->idx = 0;
                piece->rotX = Rand_Next(1);
                piece->rotY = Rand_Next(1);
                piece->rotZ = Rand_Next(1);
                piece->spinX = (Rand_Next(1) & 0x3F) - 0x20;
                piece->spinY = (Rand_Next(1) & 0x3F) - 0x20;
                piece->spinZ = (Rand_Next(1) & 0x3F) - 0x20;
                piece->flatScale = 0;
                break;
            case 3:
                piece->idx = 1;
                break;
            case 4:
                piece->idx = 2;
                break;
            }
            break;
        case 3:
            Vec_Copy(piece->pos, obj->pos);
            piece->rotZ = Rand_Next(1);
            piece->spinZ = 0xB61;
            piece->flatScale = 0;
            break;
        case 4:
            Vec_Copy(piece->pos, point);
            piece->rotZ = Rand_Next(1);
            piece->spinZ = (Rand_Next(1) & 0xFF) - 0x80;
            piece->scale *= 0.8f + 0.4f * Rand_Unit();
            break;
        case 5:
        case 6:
            piece->idx = ((s16 *) MonsterFx_GroundSprayKinds)[i];
            Vec_Copy(piece->pos, obj->pos);
            piece->pos[1] += obj->scale * ((f32 *) MonsterFx_GroundSprayRises)[i];
            switch (piece->idx) {
            case 0:
                piece->flatScale = 0;
                piece->rotY = Rand_Next(1);
                break;
            case 1:
            default:
                piece->wait = (i - 1) * -2;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0x32;
                if (randBits & (1 << i)) {
                    piece->spinZ = -piece->spinZ;
                }
                piece->speed = 10.0f;
                break;
            }
            break;
        case 7:
            if (i <= 0) {
                piece->idx = 0;
                piece->spinZ = randBits & 1;
            } else {
                piece->idx = ((randBits >> i) & 1) + 1;
                piece->scale *= 1.0f - 0.16f * (i - 1);
                piece->speed = piece->scale * ((70.0f + 20.0f * Rand_Unit()) / 8.0f);
                piece->wait = (i - 1) * -5;
            }
            Vec_Copy(piece->pos, point);
            piece->rotZ = Rand_Next(1);
            break;
        case 8:
            Vec_Copy(piece->pos, obj->pos);
            if (i <= 0) {
                piece->idx = 0;
                piece->wait = -1;
                piece->flatScale = 0;
                piece->rotY = obj->angle;
                piece->scale *= 0.7f;
            } else if (i < 2) {
                piece->idx = 1;
            } else {
                piece->idx = 2;
                piece->spinZ = 0xB6;
                if (randBits & (1 << i)) {
                    piece->spinZ = -piece->spinZ;
                }
                piece->wait = -1 - (s16) (i - 2) * 3;
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = piece->scale * ((f32 *) MonsterFx_StrikeDepths)[(s16) (i - 2)];
                Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
                Vec_Add(piece->pos, piece->pos, offset);
                piece->speed = 10.0f;
            }
            piece->rotZ = Rand_Next(1);
            break;
        case 9:
            Vec_Copy(piece->pos, obj->pos);
            if (i <= 0) {
                piece->idx = 0;
                piece->spinZ = 0x36;
            } else if (i < 4) {
                piece->idx = 1;
                piece->wait = (s16) (i - 1) * -8;
                MonsterFx_Jitter(piece, 150.0f);
                piece->speed = 10.0f;
            } else {
                piece->idx = 2;
                piece->wait = -10 - (s16) (i - 4) * 3;
                MonsterFx_Jitter(piece, 300.0f);
            }
            piece->rotZ = Rand_Next(1);
            break;
        case 10:
            Vec_Copy(piece->pos, obj->pos);
            piece->wait = -((s16 *) MonsterFx_KushalaGustDelays)[i];
            if (i <= 0) {
                piece->idx = 0;
                piece->rotX = obj->angle2;
                piece->rotY = obj->angle;
                piece->flatScale = 0;
                piece->rotZ = Rand_Next(1);
            } else if (i < 2) {
                piece->idx = 1;
            } else {
                piece->idx = 2;
                piece->rotX = obj->angle2 + (u16) (8192.0f * Rand_Unit());
                piece->rotY = obj->angle;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = (Rand_Next(1) & 0x7FF) - 0x400;
                piece->speed = -50.0f * obj->scale;
                piece->flatScale = 0;
            }
            break;
        case 13:
            if (i <= 0) {
                piece->idx = 0;
                piece->spinZ = randBits & 1;
            } else {
                piece->idx = ((randBits >> i) & 1) + 1;
                piece->scale *= 1.0f - 0.16f * (i - 1);
                piece->speed = piece->scale * ((70.0f + 20.0f * Rand_Unit()) / 8.0f);
                piece->wait = (i - 1) * -5;
            }
            Vec_Copy(piece->pos, point);
            piece->rotZ = Rand_Next(1);
            break;
        case 14:
            switch (i) {
            case 0:
                sizeRand = 0.8f + 0.4f * Rand_Unit();
                piece->scale *= sizeRand;
                piece->rotZ = 0x4000;
                break;
            case 1:
                piece->scale *= sizeRand;
                piece->sub = randBits & 3;
                break;
            case 2:
                piece->idx = ((randBits >> 2) & 1) + 2;
                piece->rotY = obj->angle;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0.5f + 65536.0f * ((5.0f + 15.0f * Rand_Unit()) / 15.0f) / 360.0f;
                if (randBits & 0x8000) {
                    piece->spinZ = -piece->spinZ;
                }
                piece->speed = (100.0f + 100.0f * Rand_Unit()) / 15.0f;
                break;
            }
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 15:
        case 17:
            MonsterFx_NextDustBurstStep(obj, piece);
            break;
        case 16:
            Vec_Copy(piece->pos, obj->pos);
            switch (i) {
            case 0:
                piece->idx = 0;
                piece->flatScale = 0;
                piece->rotY = Rand_Next(1);
                piece->spinY = 0.5f + 65536.0f * (48.0f / ((s16 *) MonsterFx_KushalaGroundWindFrames)[piece->idx]) / 360.0f;
                break;
            default:
                piece->idx = 1;
                piece->sub = i & 3;
                Vec_Copy(piece->pos, obj->pos);
                piece->spinZ = 0.5f + 65536.0f * (30.0f / ((s16 *) MonsterFx_KushalaGroundWindFrames)[piece->idx]) / 360.0f;
                piece->rotY = obj->angle + (piece->sub << 14);
                piece->scale *= 1.5f;
                piece->speed = 120.0f * piece->scale;
                break;
            }
            break;
        case 18:
            Vec_Copy(piece->pos, obj->pos);
            if (i <= 0) {
                piece->idx = 0;
                piece->rotX = obj->angle2;
                piece->rotY = obj->angle;
                piece->flatScale = 0;
                piece->rotZ = Rand_Next(1);
            } else {
                piece->idx = 1;
            }
            break;
        case 19:
            MonsterFx_NextDustKickStep(obj, piece);
            piece->pos[1] = y;
            break;
        case 20:
            Vec_Copy(piece->pos, obj->pos);
            piece->rotZ = Rand_Next(1);
            piece->spinZ = 0x5B;
            randBits = Rand_Next(1);
            if (randBits & 1) {
                piece->spinZ = -piece->spinZ;
            }
            piece->sub = (randBits >> 1) & 1;
            piece->scale *= 0.5f;
            break;
        case 21:
            piece->idx = 0;
            Vec_Copy(piece->pos, obj->pos);
            piece->wait = -4;
            offset[0] += (Rand_Next(1) & 0x3F) - 0x20;
            offset[1] += (Rand_Next(1) & 0x1F) - 0x10;
            offset[0] += (Rand_Next(1) & 0x3F) - 0x20;
            piece->pos[0] += offset[0];
            piece->pos[1] += offset[1];
            piece->pos[2] += offset[2];
            piece->rotZ = Rand_Next(1);
            piece->spinZ = 0x5B;
            randBits = Rand_Next(1);
            if (randBits & 1) {
                piece->spinZ = -piece->spinZ;
            }
            piece->sub = (randBits >> 1) & 1;
            break;
        case 22:
            Vec_Copy(piece->pos, obj->pos);
            piece->pos[0] += 100.0f * Rand_Signed();
            piece->pos[1] += 100.0f * Rand_Signed();
            piece->pos[2] += 100.0f * Rand_Signed();
            piece->rotZ = Rand_Next(1);
            piece->spinZ = (Rand_Next(1) & 0x1FF) - 0x100;
            piece->scale = 2.5f + 0.5f * Rand_Unit();
            piece->speed = 2.0f;
            break;
        case 23:
            Vec_Copy(piece->pos, obj->pos);
            piece->idx = 0;
            piece->rotY = obj->angle;
            piece->flatScale = 0;
            piece->sub = obj->var2;
            break;
        case 24:
            Vec_Copy(piece->pos, obj->pos);
            piece->idx = ((u8 *) MonsterFx_RajangRageStartKinds)[i];
            piece->flatScale = 0;
            piece->rotY = obj->angle;
            switch (piece->idx) {
            case 1:
                piece->rotY += randBits >> 1;
                if (i & 1) {
                    piece->rotY += 0x7FFF + 1;
                }
                piece->spinY = -0x2D7;
                break;
            }
            break;
        case 25:
            Vec_Copy(piece->pos, obj->pos);
            piece->rotX = 0xF05C;
            piece->rotY = obj->angle;
            piece->scale = 0.2f;
            switch (piece->idx) {
            case 0:
                piece->flatScale = 0;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0x12;
                break;
            case 1:
                break;
            }
            break;
        case 26:
            Vec_Copy(piece->pos, obj->pos);
            piece->wait = -((s16 *) MonsterFx_ShenGaorenPuffDelays)[i];
            switch (piece->idx) {
            case 0:
                break;
            case 1:
                piece->flatScale = 0;
                piece->rotX = 0xE800;
                piece->rotY = obj->angle;
                piece->rotZ = Rand_Next(1);
                break;
            case 2:
                piece->flatScale = 0;
                piece->rotX = 0xE800;
                piece->rotY = obj->angle;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0xAAB;
                break;
            }
            break;
        case 27:
            Vec_Copy(piece->pos, obj->pos);
            piece->rotZ = Rand_Next(1);
            piece->spinZ = (Rand_Next(1) & 0xFF) - 0x80;
            piece->scale *= 5.0f + 1.5f * Rand_Signed();
            break;
        case 28:
            piece->idx = Rand_Next(1) % 3;
            Vec_Copy(piece->pos, obj->pos);
            piece->rotZ = Rand_Next(1);
            piece->spinZ = 0x5B0;
            piece->scale *= 0.8f + 0.2f * Rand_Unit();
            break;
        case 29:
            if (i < 2) {
                piece->idx = MonsterFx_TescaAuraKinds[obj->var2];
                piece->wait = obj->var1 - MonsterFx_TescaAuraStarts[obj->var2];
                obj->var2++;
            } else {
                piece->idx = 0xFF;
            }
            break;
        case 30:
            piece->flatScale = 0;
            piece->wait = ((s16 *) MonsterFx_RangedSplashDelays)[i];
            Vec_Copy(piece->pos, obj->pos);
            switch (piece->idx) {
            case 0:
            case 1:
                piece->speed = 40.0f + 20.0f * Rand_Signed();
                piece->spinY = 0.5f + 65536.0f * (20.0f * Rand_Signed()) / 360.0f;
                piece->rotZ = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                piece->riseSpeed = 30.0f;
                break;
            case 2:
            case 3:
                piece->speed = 15.0f + 10.0f * Rand_Signed();
                piece->rotZ = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                piece->riseSpeed = 20.0f;
                break;
            case 4:
            case 5:
            case 6:
                piece->rotZ = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                piece->spinZ = 0.5f + 65536.0f * ((20.0f * Rand_Signed()) / ((s16 *) MonsterFx_RangedSplashFrames)[piece->idx]) / 360.0f;
                piece->sub = (Rand_Next(1) % 2) * 14;
                piece->pos[0] += 100.0f * Rand_Signed();
                piece->pos[1] += 100.0f * Rand_Signed();
                piece->pos[2] += 100.0f * Rand_Signed();
                break;
            }
            switch (piece->idx) {
            case 0:
            case 1:
                piece->scale = 0.5f + 0.5f * Rand_Unit();
                break;
            case 2:
            case 3:
            case 4:
                piece->scale = 1.0f;
                break;
            }
            break;
        case 31:
            piece->rotZ = Rand_Next(1);
            piece->speed = -10.0f * Rand_Unit();
            piece->pos[0] = obj->pos[0] + 50.0f * Rand_Signed();
            piece->pos[1] = obj->pos[1] + 50.0f * Rand_Signed();
            piece->pos[2] = obj->pos[2] + 50.0f * Rand_Signed();
            break;
        case 32:
            Vec_Copy(piece->pos, obj->pos);
            piece->idx = ((u8 *) MonsterFx_ChameleosImpactKinds)[i];
            piece->wait = -((s16 *) D_008D7318)[i];
            switch (piece->idx) {
            case 0:
            case 1:
                piece->flatScale = 0;
                piece->rotY = Rand_Next(1);
                break;
            case 2:
                piece->rotZ = Rand_Next(1);
                piece->sub = quarters[quartersUsed++];
                piece->speed = piece->scale * ((300.0f + 100.0f * Rand_Unit()) / ((u16 *) MonsterFx_ChameleosImpactFrames)[piece->idx]);
                break;
            case 3:
                piece->flatScale = 0;
                break;
            }
            break;
        case 33:
            Vec_Copy(piece->pos, obj->pos);
            piece->idx = ((u8 *) MonsterFx_ChameleosGroundBurstKinds)[i];
            piece->wait = -((s16 *) MonsterFx_ChameleosGroundBurstDelays)[i];
            switch (piece->idx) {
            case 0:
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0x1555 / ((u16 *) MonsterFx_ChameleosGroundBurstFrames)[piece->idx];
                if (randBits & 1) {
                    piece->spinZ = -piece->spinZ;
                }
                piece->sub = quarters[quartersUsed++];
                piece->speed = piece->scale * ((400.0f + 100.0f * Rand_Unit()) / ((u16 *) MonsterFx_ChameleosGroundBurstFrames)[piece->idx]);
                break;
            case 1:
                piece->rotX = 0x4000;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0x1555 / ((u16 *) MonsterFx_ChameleosGroundBurstFrames)[piece->idx];
                if (randBits & 1) {
                    piece->spinZ = -piece->spinZ;
                }
                piece->pos[1] += 30.0f * piece->scale;
                break;
            case 2:
                piece->pos[0] += 400.0f * Rand_Signed();
                piece->pos[2] += 400.0f * Rand_Signed();
                piece->rotZ = Rand_Next(1);
                break;
            }
            break;
        case 34:
            piece->idx = 0;
            if (obj->var2 == 0) {
                if (i > 0) {
                    piece->wait = 0x28;
                } else {
                    piece->wait = i * -5;
                }
            } else {
                piece->wait = i * -5;
            }
            piece->flatScale = 0;
            piece->speed = 4.0f;
            break;
        case 35:
            Vec_Copy(piece->pos, obj->pos);
            piece->idx = Rand_Next(1) & 3;
            piece->flatScale = 0;
            break;
        case 36:
            Vec_Copy(piece->pos, obj->pos);
            piece->flatScale = 0;
            piece->rotX = obj->angle2;
            piece->rotY = obj->angle;
            piece->rotZ = Rand_Next(1);
            break;
        case 37:
            Vec_Copy(piece->pos, obj->pos);
            switch (piece->idx) {
            case 0:
                break;
            case 1:
            case 2:
                piece->rotX = obj->angle2;
                piece->rotY = obj->angle;
                break;
            }
            break;
        case 38:
            piece->wait = -((s16 *) MonsterFx_FatalisFireballDelays)[i];
            switch (piece->idx) {
            case 0:
                break;
            case 1:
            case 2:
                piece->rotX = obj->angle2;
                piece->rotY = obj->angle;
                piece->rotZ = Rand_Next(1);
                piece->spinZ = 0xB6;
                piece->flatScale = 0;
                break;
            }
            break;
        case 39:
            Vec_Copy(piece->pos, obj->pos);
            piece->pos[0] += obj->scale * (50.0f * Rand_Signed());
            piece->pos[2] += obj->scale * (50.0f * Rand_Signed());
            piece->idx = randBits & 1;
            piece->rotZ = randBits;
            piece->spinZ = 0x5B;
            if ((randBits >> 1) & 1) {
                piece->spinZ = -piece->spinZ;
            }
            break;
        case 40:
            piece->wait = i * -3;
            piece->idx = (randBits >> i) & 1;
            piece->rotX = Rand_Next(1);
            piece->rotY = Rand_Next(1);
            piece->rotZ = Rand_Next(1);
            piece->flatScale = 0;
            break;
        }
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = MonsterFx_Draw;
        } else {
            piece->node = NULL;
        }
    }
}

/*
 * Runs a MonsterFx object each frame: counts its timer and ends it, hidden,
 * past the type's lifetime (gMonsterFx_Lifetimes; 1000: none); otherwise runs
 * the chain (MonsterFx_RunChain) or the parts (MonsterFx_RunSeq).
 */
void MonsterFx_Run(StageObj *obj)
{
    s16 limit = gStageObjK1C_Limits[obj->type];

    obj->timer++;
    if (limit != 1000 && limit < obj->timer) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    switch (obj->type) {
    case 0:
    case 0xB:
        MonsterFx_RunChain(obj);
        break;
    default:
        MonsterFx_RunSeq(obj);
        break;
    }
}

/*
 * Returns the world matrix of the owner's bone 0x25, the bone the breath
 * comes from (likely the mouth). MonsterFx_RunChain reads it through this
 * inline helper, not a plain call: so the matrix pointer gets the stack slot
 * it has in the game's code.
 */
static inline f32 *MonsterFx_BreathBone(Em *em)
{
    return (f32 *)Em_Bone(em, 0x25);
}

/*
 * Runs a breath stream. While the owner is active, in a breath motion and
 * under 300 frames, each link trails the one before it from the breath bone
 * (MonsterFx_FollowBone), its z angle spinning, and the owner's sound 0x42
 * plays every 10 frames. Then the links let go one after another: each takes
 * a random spin and rises 2 a frame for 30 frames, by its scale curve and
 * colour keys; the object ends 0x26 frames after the release (counted in the
 * halfword just past the links).
 */
void MonsterFx_RunChain(StageObj *obj)
{
    f32 lastLinkPos[3];
    MonsterFxLink *first;
    u8 moved = 0;
    MonsterFxLink *link = (MonsterFxLink *) obj->buf;
    Em *em = obj->owner;
    s16 i;
    s16 linkCount = gStageObjK1C_Counts[obj->type];
    s16 scaleCurve = gStageObjK1C_ScaleCurveIdx[obj->type];
    s16 colorCurve = gStageObjK1C_ColorCurveIdx[obj->type];
    MonsterFxLink *prev;
    DrawSlot *node;
    f32 *boneMtx;

    switch (obj->type) {
    case 0:
        first = link;
        switch (obj->var0) {
        case 0:
            if (MonsterFx_OwnerActive(em) == 0 || (em->motionId[0] != 0x417 && em->motionId[0] != 0x41A) || obj->timer >= 300) {
                *(s16 *) ((u8 *) link + 0x320) = 0;
                Vec_Copy(link->pos, link->base[0]);
                obj->var0++;
            } else {
                boneMtx = MonsterFx_BreathBone(em);
            }
            break;
        case 1:
            if (++*(s16 *) ((u8 *) link + 0x320) >= 0x26) {
                obj->state++;
                obj->visible = 0;
                return;
            }
            break;
        }
        break;
    }
    for (i = 0; i < linkCount; i++, link++) {
        link->frame++;
        switch (obj->type) {
        case 0:
            switch (obj->var0) {
            case 0:
                MonsterFx_FollowBone(obj, link, prev, boneMtx, first->dir[0], 1);
                prev = link;
                link->rotZ += link->spinZ;
                Vec_Copy((f32 *) &lastLinkPos, link->pos);
                moved = 1;
                break;
            case 1:
            default:
                if (++link->releaseFrame == 0) {
                    MonsterFx_RandomSpin(link);
                }
                if (link->releaseFrame >= 30) {
                    continue;
                }
                if (link->releaseFrame >= 0) {
                    Key_LerpVec2(gStageObjK1C_ScaleCurves[scaleCurve], link->sc, link->releaseFrame);
                    Color_EvalKeys((s32 *)gStageObjK1C_ColorKeys[colorCurve], (f32) link->releaseFrame, (u32 *)&link->color);
                    link->pos[1] += 2.0f;
                }
                break;
            case 2:
                if (++link->releaseFrame == 0) {
                    if (obj->var2 != 0xFF) {
                        if (MonsterFx_OwnerActive(em) == 1) {
                            Em_BonePos(em, 0x25, link->pos);
                        } else {
                            obj->var2 = 0xFF;
                            link->releaseFrame = 30;
                        }
                    } else {
                        link->releaseFrame = 30;
                    }
                }
                if (link->releaseFrame < 30 && link->releaseFrame > 0) {
                    Key_LerpVec2(gStageObjK1C_ScaleCurves[scaleCurve], link->sc, link->releaseFrame);
                    Color_EvalKeys((s32 *)gStageObjK1C_ColorKeys[colorCurve], (f32) link->releaseFrame, (u32 *)&link->color);
                    link->pos[1] += 2.0f;
                } else {
                    continue;
                }
                break;
            }
            break;
        }
        if ((node = link->node) != NULL) {
            Vec_Copy(node->pos, link->pos);
            OT_AddByDepth(gDrawOtSorted, link->node, 0x40, 0);
        }
    }
    if (moved & 0xFF) {
        switch (obj->type) {
        case 0:
            if (obj->timer % 10 == 0) {
                Em_PlaySeChecked((struct Em *)em, 0x42, 0, (f32 *) &lastLinkPos, 6, 0);
            }
            break;
        }
    }
}

extern u32 gStageObjK1C_SeqTbls[];
s32 EmKushala_StateHasWindRing(Em *em);
s32 EmKushala_WindActive(Em *em);
s32 EmKushala_WindActiveTesca(Em *em);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
s32 MonsterFx_AllPartsDone(StageObj *o);
void MonsterFx_RestartKushalaRing(StageObj *o);
void MonsterFx_RestartTescaAuraBursts(StageObj *o);
s32 MonsterFx_TescaAuraBurstsDone(StageObj *o);
static void MonsterFx_NextKushalaRingStep(StageObj *o, MonsterFxPiece *w);
static void MonsterFx_NextTescaAuraStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_StartKushalaRingStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_StartTescaAuraStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_StartDustBurstStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_StartDustKickStep(StageObj *o, MonsterFxPiece *w);
void MonsterFx_CreateMoving(f32 *pos, f32 *pos2, u8 k, f32 scale);

/*
 * Runs the parts of a non-chain MonsterFx object. First the type's own work:
 * following a bone, making new objects (MFX_BREATH_BEAM's motes,
 * MFX_BLANGONGA_STRIKE's spray), ending when the owner is gone, and the two
 * auras, which restart while the owner's wind is up. Then each part counts
 * its frame (wait); past its step's length (gMonsterFx_StepFrames) a step-
 * driven type moves it to its next step. A part inside its step takes its
 * scale from a curve and its colour from colour keys (or its alpha from a
 * curve), moves as its type says, turns by its spins and goes into the depth-
 * sorted draw table.
 */
void MonsterFx_RunSeq(StageObj *obj)
{
    f32 alphaKey;
    f32 point[3];
    f32 offset[3];
    f32 axis[3];
    f32 ownerPos[3];
    s16 i;
    s16 limit;
    u16 ownerYaw;
    u8 restartParts;
    s32 loopCount;
    MonsterFxPiece *piece = (MonsterFxPiece *) obj->buf;
    Em *em = obj->owner;
    s16 partCount = gStageObjK1C_Counts[obj->type];
    s16 scaleCurve = gStageObjK1C_ScaleCurveIdx[obj->type];
    s16 colorCurve = gStageObjK1C_ColorCurveIdx[obj->type];
    s16 *stepFrames = (s16 *) gStageObjK1C_SeqTbls[obj->type];
    f32 *boneMtx;
    f32 k;
    f32 *scaleKey;
    s32 w;
    u8 *colorKey;

    switch (obj->type) {
    case 3:
        restartParts = 0;
        switch (obj->var0) {
        case 0:
            if (obj->var2 != 0xFF) {
                if (MonsterFx_OwnerActive(em) == 0) {
                    obj->var2 = 0xFF;
                } else {
                    boneMtx = (f32 *)Em_Bone(em, 0x25);
                    Mtx_GetTrans(obj->pos, boneMtx);
                }
            }
            limit = 10;
            if (obj->timer >= 10) {
                restartParts = 1;
                obj->var0++;
                limit = 200;
            }
            break;
        case 1:
            if (obj->var2 != 0xFF) {
                if (MonsterFx_OwnerActive(em) == 0) {
                    obj->var2 = 0xFF;
                } else {
                    boneMtx = (f32 *)Em_Bone(em, 0x25);
                    Mtx_GetTrans(obj->pos, boneMtx);
                }
            }
            limit = 200;
            if (obj->timer >= 210) {
                restartParts = 1;
                obj->var0++;
                obj->var2 = 0xFF;
                limit = 30;
            }
            break;
        case 2:
        default:
            scaleCurve++;
            colorCurve++;
            limit = 30;
            break;
        }
        if (!(obj->timer & 3)) {
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 1.0f;
            Vec_RotateX(offset, 6.2831855f * piece->rotX / 65536.0f);
            Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
            point[0] = piece->pos[0] + 1000.0f * offset[0] * obj->scale;
            point[1] = piece->pos[1] + 1000.0f * offset[1] * obj->scale;
            point[2] = piece->pos[2] + 1000.0f * offset[2] * obj->scale;
            MonsterFx_CreateMoving(point, offset, 4, obj->scale);
        }
        break;
    case 4:
        limit = 0x1A;
        break;
    case 7:
    case 0xD:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        break;
    case 8:
        if (obj->timer == 5) {
            MonsterFx_CreateAtYaw(obj->pos, obj->angle, 9, obj->scale);
        }
        break;
    case 0xC:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (!(u8)(s32)Player_InCurrentArea(em)) {
            return;
        }
        Em_BonePos(em, 2, ownerPos);
        ownerYaw = em->rotY;
        switch (obj->var0) {
        case 0:
            if (++obj->var1 == 7) {
                Vec_Copy(obj->pos, ownerPos);
                obj->pos[1] = em->floorY;
                obj->angle2 = em->rotY;
                if (obj->pos[1] < em->posY - 1000.0f || EmKushala_StateHasWindRing(em) == 0) {
                    obj->ownerIndex = 1;
                } else {
                    EmImpactAttack_CreateAt(em, obj->pos, 3);
                    Em_PlaySeChecked((struct Em *)em, 0x40, 0, obj->pos, 6, 0);
                }
            }
            if (MonsterFx_AllPartsDone(obj) == 1) {
                obj->var0++;
                obj->var1 = 0;
                obj->ownerIndex = 0;
                obj->angle = em->rage;
                return;
            }
            break;
        case 1:
            if (EmKushala_WindActive(em) == 0) {
                return;
            }
            switch (em->motionId[0]) {
            case 0x408:
            case 0x416:
            case 0x417:
            case 0x41A:
            case 0x429:
            case 0x42C:
            case 0x42D:
                return;
            }
            if (++obj->var1 >= 0) {
                obj->scale = em->scaleX;
                obj->var0 = 0;
                obj->var1 = 0;
                MonsterFx_RestartKushalaRing(obj);
                if (Player_InCurrentArea(em)) {
                    Em_PlaySeChecked((struct Em *)em, 0x3F, 0, ownerPos, 6, 0);
                }
            }
            return;
        }
        break;
    case 0xF:
    case 0x11:
    case 0x13:
        if (MonsterFx_AllPartsDone(obj) == 1) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        break;
    case 0x18:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        Em_BonePos(em, 4, obj->pos);
        obj->pos[1] -= 100.0f;
        break;
    case 0x19:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x3F5 && em->motionId[0] != 0x413) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        MonsterFx_PlaceBehindOwner(obj);
        break;
    case 0x1D:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (!(u8)(s32)Player_InCurrentArea(em)) {
            return;
        }
        Em_BonePos(em, 2, ownerPos);
        ownerYaw = em->rotY;
        switch (obj->var0) {
        case 0:
        again:
            if (EmKushala_WindActiveTesca(em) == 0) {
                return;
            }
            switch (em->motionId[0]) {
            case 0x408:
            case 0x416:
            case 0x417:
            case 0x41A:
            case 0x429:
            case 0x42C:
            case 0x42D:
                return;
            }
            if (EmKushala_StateHasWindRing(em) == 0) {
                break;
            }
            obj->var0++;
            MonsterFx_RestartTescaAuraBursts(obj);
            Vec_Copy(obj->pos, ownerPos);
            obj->angle2 = em->rotY;
        case 1:
            if (MonsterFx_TescaAuraBurstsDone(obj) == 1) {
                obj->scale = em->scaleX;
                obj->var0 = 0;
                obj->var1 = 0;
                obj->angle = 0;
                goto again;
            }
            break;
        }
        break;
    case 0x23:
        limit = 4;
        break;
    case 0x26:
        boneMtx = (f32 *)Em_Bone(em, 0x33);
        offset[0] = 0.0f;
        offset[1] = -91.6f;
        offset[2] = 132.8f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(point, boneMtx);
        obj->pos[0] = point[0] + offset[0];
        obj->pos[1] = point[1] + offset[1];
        obj->pos[2] = point[2] + offset[2];
        break;
    case 0x28:
        if (MonsterFx_OwnerActive(em) == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        MonsterFx_GetOwnerBone(obj, obj->pos);
        break;
    }
    loopCount = partCount;
    for (i = 0; i < loopCount; i++, piece++) {
        switch (obj->type) {
        case 3:
            if (restartParts) {
                piece->wait = 0;
            }
            if (obj->var2 != 0xFF) {
                Vec_DirToAngles(boneMtx + 8, (s16 *) &piece->rotX, (s16 *) &piece->rotY);
            }
            Vec_Copy(piece->pos, obj->pos);
            piece->wait++;
            switch (obj->var0) {
            case 1:
                goto move;
            case 0:
            case 2:
                if (piece->wait <= 0) {
                    continue;
                }
                if (piece->wait > limit) {
                    goto move;
                }
                scaleCurve = gStageObjK1C_ScaleCurveIdx[obj->type];
                colorCurve = gStageObjK1C_ColorCurveIdx[obj->type];
                if (obj->var0 == 2) {
                    scaleCurve++;
                    colorCurve++;
                }
                goto curve;
            }
            break;
        case 12:
        case 29:
            if (piece->idx == 0xFF) {
                continue;
            }
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type];
            colorCurve = piece->idx + gStageObjK1C_ColorCurveIdx[obj->type];
            break;
        case 15:
        case 17:
        case 19:
            if (piece->idx == 0xFF) {
                continue;
            }
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type];
            colorCurve = piece->idx + gStageObjK1C_ColorCurveIdx[obj->type];
            break;
        case 10: case 14: case 16: case 18: case 20: case 21: case 22: case 24: case 25: case 26:
        case 30: case 31: case 32: case 33: case 34: case 36: case 37: case 38: case 39: case 40:
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type];
            colorCurve = piece->idx + gStageObjK1C_ColorCurveIdx[obj->type];
            break;
        case 23:
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type];
            colorCurve = piece->sub + gStageObjK1C_ColorCurveIdx[obj->type];
            break;
        case 7:
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type] + obj->var1 * 3;
            colorCurve = piece->idx + gStageObjK1C_ColorCurveIdx[obj->type] + obj->var1 * 3;
            break;
        case 13:
            scaleCurve = piece->idx + gStageObjK1C_ScaleCurveIdx[obj->type] + obj->var1 * 3;
            colorCurve = piece->idx + gStageObjK1C_ColorCurveIdx[obj->type] + obj->var1 * 3;
            break;
        default:
            if (stepFrames == NULL) {
                scaleCurve = gStageObjK1C_ScaleCurveIdx[obj->type];
                colorCurve = gStageObjK1C_ColorCurveIdx[obj->type];
            }
            break;
        }
        if (stepFrames != NULL) {
            limit = stepFrames[piece->idx];
        }
        if (++piece->wait > limit) {
            switch (obj->type) {
            case 9:
                switch (piece->idx) {
                case 2:
                    if (obj->timer < 0x24) {
                        piece->wait = -1;
                        Vec_Copy(piece->pos, obj->pos);
                        MonsterFx_Jitter(piece, 300.0f);
                    }
                    break;
                }
                break;
            case 0xC:
                MonsterFx_NextKushalaRingStep(obj, piece);
                if (piece->idx == 0xFF) {
                    continue;
                }
                if (stepFrames != NULL) {
                    limit = stepFrames[piece->idx];
                }
                break;
            case 0xF:
            case 0x11:
                MonsterFx_NextDustBurstStep(obj, piece);
                if (piece->idx == 0xFF) {
                    continue;
                }
                if (stepFrames != NULL) {
                    limit = stepFrames[piece->idx];
                }
                break;
            case 0x13:
                MonsterFx_NextDustKickStep(obj, piece);
                if (piece->idx == 0xFF) {
                    continue;
                }
                if (stepFrames != NULL) {
                    limit = stepFrames[piece->idx];
                }
                break;
            case 0x19:
                piece->wait = 1;
                break;
            case 0x1D:
                MonsterFx_NextTescaAuraStep(obj, piece);
                if (piece->idx == 0xFF) {
                    continue;
                }
                if (stepFrames != NULL) {
                    limit = stepFrames[piece->idx];
                }
                break;
            case 0x21:
                switch (piece->idx) {
                case 2:
                    if (obj->timer < 0x34) {
                        piece->wait = 1;
                        piece->rotZ = Rand_Next(1);
                        piece->pos[0] = obj->pos[0] + 400.0f * Rand_Signed();
                        piece->pos[2] = obj->pos[2] + 400.0f * Rand_Signed();
                    }
                    break;
                }
                break;
            }
        }
        if (piece->wait <= 0 || limit < piece->wait) {
            if (piece->useColorKeys) {
                colorCurve++;
                scaleCurve++;
            } else {
                scaleCurve += 2;
            }
            continue;
        }
        if (piece->wait == 1) {
            switch (obj->type) {
            case 7:
                boneMtx = (f32 *)Em_Bone(em, 0x12);
                offset[0] = 0.0f;
                offset[1] = -70.0f;
                offset[2] = 42.0f;
                Vec_RotateSelf(offset, boneMtx);
                Mtx_GetTrans(point, boneMtx);
                piece->pos[0] = point[0] + offset[0];
                piece->pos[1] = point[1] + offset[1];
                piece->pos[2] = point[2] + offset[2];
                Vec_Copy(axis, boneMtx + 4);
                axis[0] = -axis[0];
                axis[1] = -axis[1];
                axis[2] = -axis[2];
                Vec_DirToAngles(axis, (s16 *) &piece->rotX, (s16 *) &piece->rotY);
                piece->rotX = piece->rotX + 0x7FFF + 0x7001;
                break;
            case 0xC:
                MonsterFx_StartKushalaRingStep(obj, piece);
                break;
            case 0xD:
                switch (em->type) {
                case EM_KIND_CHAMELEOS:
                    boneMtx = (f32 *)Em_Bone(em, 0x29);
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 30.0f;
                    break;
                case EM_KIND_BULLDROME:
                    boneMtx = (f32 *)Em_Bone(em, 0xB);
                    offset[0] = 0.0f;
                    offset[1] = -10.0f;
                    offset[2] = 20.0f;
                    break;
                default:
                    boneMtx = (f32 *)Em_Bone(em, 0x27);
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 100.0f;
                    break;
                }
                Vec_RotateSelf(offset, boneMtx);
                Mtx_GetTrans(point, boneMtx);
                piece->pos[0] = point[0] + offset[0];
                piece->pos[1] = point[1] + offset[1];
                piece->pos[2] = point[2] + offset[2];
                Vec_Copy(axis, boneMtx + 8);
                Vec_DirToAngles(axis, (s16 *) &piece->rotX, (s16 *) &piece->rotY);
                piece->rotX = piece->rotX + 0x7FFF + 0x7001;
                break;
            case 0xF:
            case 0x11:
                MonsterFx_StartDustBurstStep(obj, piece);
                break;
            case 0x13:
                MonsterFx_StartDustKickStep(obj, piece);
                break;
            case 0x1D:
                MonsterFx_StartTescaAuraStep(obj, piece);
                break;
            case 0x22:
                boneMtx = (f32 *)Em_Bone(em, 0xE);
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 0.0f;
                Vec_RotateSelf(offset, boneMtx);
                Mtx_GetTrans(point, boneMtx);
                piece->pos[0] = point[0] + offset[0];
                piece->pos[1] = point[1] + offset[1];
                piece->pos[2] = point[2] + offset[2];
                break;
            }
        }
    curve:
        switch (obj->type) {
        case 0x19:
            k = (obj->timer & 3) + 1;
            break;
        default:
            k = piece->wait;
            break;
        }
        if (piece->flatScale) {
            scaleKey = gStageObjK1C_ScaleCurves[scaleCurve++];
            Key_LerpVec2(scaleKey, piece->sc, k);
        } else {
            scaleKey = gStageObjK1C_ScaleCurves[scaleCurve++];
            Key_LerpVec3(scaleKey, piece->sc, k);
        }
        w = piece->wait;
        if (piece->useColorKeys == 0) {
            scaleKey = gStageObjK1C_ScaleCurves[scaleCurve++];
            Key_LerpFloat(scaleKey, &alphaKey, w);
            piece->color = ((u8) (255.0f * alphaKey) << 24) | 0xFFFFFF;
        } else {
            colorKey = gStageObjK1C_ColorKeys[colorCurve++];
            Color_EvalKeys((s32 *)colorKey, w, (u32 *)&piece->color);
        }
    move:
        switch (obj->type) {
        case 1:
            switch (piece->idx) {
            case 4:
                    piece->pos[1] += 6.0f;
                break;
            }
            break;
        case 4:
            piece->pos[0] += 15.0f * obj->pos[0];
            piece->pos[1] += 15.0f * obj->pos[1];
            piece->pos[2] += 15.0f * obj->pos[2];
            break;
        case 5:
        case 6:
            switch (piece->idx) {
            case 0:
                break;
            case 1:
            default:
                piece->speed += -2.5f;
                piece->pos[1] += piece->speed;
                break;
            }
            break;
        case 7:
        case 13:
            switch (piece->idx) {
            case 0:
                break;
            default:
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = piece->speed;
                    Vec_RotateX(offset, 6.2831855f * piece->rotX / 65536.0f);
                    Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                    Vec_Add(piece->pos, piece->pos, offset);
                break;
            }
            break;
        case 8:
            switch (piece->idx) {
            case 2:
                    piece->speed += -1.0f;
                    piece->pos[1] += piece->speed;
                break;
            }
            break;
        case 9:
            switch (piece->idx) {
            case 1:
                    piece->speed += -1.0f;
                    piece->pos[1] += piece->speed;
                break;
            }
            break;
        case 10:
        case 18:
            switch (piece->idx) {
            case 2:
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = piece->speed;
                    Vec_RotateX(offset, 6.2831855f * obj->angle2 / 65536.0f);
                    Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                    piece->pos[0] += offset[0];
                    piece->pos[1] += offset[1];
                    piece->pos[2] += offset[2];
                break;
            }
            break;
        case 12:
            switch (piece->idx) {
            case 0:
                Vec_Copy(piece->pos, ownerPos);
                piece->rotY = ownerYaw;
                break;
            case 1:
                if (obj->ownerIndex != 0) {
                    continue;
                }
                break;
            case 2:
            default:
                if (obj->ownerIndex != 0) {
                    continue;
                }
                piece->speed *= 0.9f;
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = piece->speed;
                Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                piece->pos[0] += offset[0];
                piece->pos[1] += 10.0f * obj->scale;
                piece->pos[2] += offset[2];
                break;
            }
            break;
        case 14:
            switch (piece->idx) {
            case 2:
            case 3:
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = piece->speed;
                Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                piece->pos[0] += offset[0];
                piece->pos[2] += offset[2];
                break;
            }
            break;
        case 15:
        case 17:
            switch (piece->idx) {
            case 3:
                piece->speed += -2.0f;
                piece->pos[1] += piece->speed;
                break;
            case 4:
            case 5:
                piece->riseSpeed += -8.0f;
                offset[0] = 0.0f;
                offset[1] = piece->riseSpeed;
                offset[2] = piece->speed;
                Vec_RotateY(offset, 6.2831855f * (obj->angle + (piece->sub << 14)) / 65536.0f);
                piece->pos[0] += offset[0];
                piece->pos[1] += offset[1];
                piece->pos[2] += offset[2];
                break;
            case 6:
            case 7:
                piece->speed *= 0.95f;
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = piece->speed;
                Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                piece->pos[0] += offset[0];
                piece->pos[1] += 8.0f * obj->scale;
                piece->pos[2] += offset[2];
                break;
            }
            break;
        case 16:
            switch (piece->idx) {
            case 0:
                break;
            default:
                    piece->speed *= 0.9f;
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = piece->speed;
                    Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                    piece->pos[0] += offset[0];
                    piece->pos[1] += 10.0f * obj->scale;
                    piece->pos[2] += offset[2];
                break;
            }
            break;
        case 19:
            switch (piece->idx) {
            case 0:
                break;
            default:
                    piece->speed *= 0.95f;
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = piece->speed;
                    Vec_RotateY(offset, 6.2831855f * piece->rotY / 65536.0f);
                    piece->pos[0] += offset[0];
                    piece->pos[1] += 3.0f * piece->scale;
                    piece->pos[2] += offset[2];
                break;
            }
            break;
        case 20:
        case 21:
            piece->pos[1] += 5.0f * piece->scale;
            break;
        case 22:
            piece->speed += -2.0f;
            piece->pos[1] += piece->speed;
            break;
        case 24:
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 25:
            Vec_Copy(piece->pos, obj->pos);
            piece->rotY = obj->angle;
            piece->scale += 0.01f * (1.0f - piece->scale);
            if (piece->scale > 1.0f) {
                piece->scale = 1.0f;
            }
            break;
        case 27:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = -7.5f;
            Vec_RotateX(offset, 6.2831855f * obj->angle2 / 65536.0f);
            Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
            piece->pos[0] += offset[0];
            piece->pos[1] += offset[1];
            piece->pos[2] += offset[2];
            break;
        case 28:
            if (piece->wait < 0xB) {
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 25.0f * piece->scale;
                Vec_RotateX(offset, 6.2831855f * obj->angle2 / 65536.0f);
                Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
                piece->pos[0] += offset[0];
                piece->pos[1] += offset[1];
                piece->pos[2] += offset[2];
            } else {
                piece->spinZ = 0;
            }
            break;
        case 29:
            switch (piece->idx) {
            case 0:
                Vec_Copy(piece->pos, ownerPos);
                break;
            case 1:
                Vec_Copy(piece->pos, ownerPos);
                piece->rotY = ownerYaw;
                break;
            case 2:
            case 3:
            default:
                piece->pos[1] += piece->speed;
                break;
            }
            break;
        case 30:
            switch (piece->idx) {
            case 0:
            case 1:
            case 2:
            case 3:
                if (piece->idx < 2) {
                    piece->riseSpeed = piece->riseSpeed - 3.0f;
                } else {
                    piece->riseSpeed = piece->riseSpeed - 1.5f;
                }
                piece->speed *= 0.95f;
                axis[0] = 0.0f;
                axis[1] = piece->riseSpeed;
                axis[2] = piece->speed;
                Vec_RotateY(axis, 6.2831855f * (obj->angle + piece->spinY) / 65536.0f);
                piece->pos[0] += axis[0];
                piece->pos[1] += axis[1];
                piece->pos[2] += axis[2];
                break;
            case 4:
            case 5:
            case 6:
                piece->pos[1] = 100.0f + obj->pos[1];
                break;
            }
            break;
        case 31:
            piece->speed = piece->speed - 2.0f;
            piece->pos[1] += piece->speed;
            break;
        case 32:
            switch (piece->idx) {
            case 2:
                    offset[0] = 0.0f;
                    offset[1] = 3.0f - 2.5f * piece->wait;
                    offset[2] = piece->speed;
                    Vec_RotateY(offset, 6.2831855f * (piece->sub << 14) / 65536.0f);
                    piece->pos[0] += offset[0];
                    piece->pos[1] += offset[1];
                    piece->pos[2] += offset[2];
                break;
            }
            break;
        case 33:
            switch (piece->idx) {
            case 0:
                    offset[0] = 0.0f;
                    offset[1] = 12.0f - piece->wait;
                    offset[2] = piece->speed;
                    Vec_RotateY(offset, 6.2831855f * (piece->sub << 14) / 65536.0f);
                    piece->pos[0] += offset[0];
                    piece->pos[1] += offset[1];
                    piece->pos[2] += offset[2];
                break;
            }
            break;
        case 34:
            if (obj->var2 == 0) {
                piece->speed = piece->speed - 4.0f;
            } else {
                piece->speed = piece->speed - 6.0f;
            }
            piece->pos[1] += piece->speed;
            if (piece->pos[1] < obj->pos[1]) {
                piece->wait = limit;
                Em_PlaySeChecked((struct Em *)em, 0x32, 0, piece->pos, 6, 0);
                continue;
            }
            break;
        case 39:
            piece->pos[1] += 15.0f * obj->scale;
            break;
        case 38:
        case 40:
            Vec_Copy(piece->pos, obj->pos);
        case 0:
        case 2:
        case 3:
        case 11:
        case 23:
        case 26:
        case 35:
        case 36:
        case 37:
            break;
        }
        piece->rotX += piece->spinX;
        piece->rotY += piece->spinY;
        piece->rotZ += piece->spinZ;
        if (piece->node != NULL) {
            Vec_Copy(piece->node->pos, piece->pos);
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
    }
}

/*
 * Moves the object to its free state and frees its parts' draw slots
 * (MonsterFx_EndChain or MonsterFx_EndSeq).
 */
void MonsterFx_End(StageObj *obj)
{
    s16 partCount;

    obj->state++;
    partCount = gStageObjK1C_Counts[obj->type];
    switch (obj->type) {
    case 0:
    case 0xB:
        MonsterFx_EndChain(obj, partCount);
        break;
    default:
        MonsterFx_EndSeq(obj, partCount);
        break;
    }
}

/*
 * Frees the draw slots of the n links of a chain (MonsterFxLink, 0x64 bytes
 * each).
 */
void MonsterFx_EndChain(StageObj *obj, s16 linkCount)
{
    u8 *bytes = obj->buf;
    s16 i;

    for (i = 0; i < linkCount; i++) {
        if (*(s32 *) (bytes + 0x20) != 0) {
            DrawSlot_Free(*(s16 *) bytes);
        }
        bytes += 0x64;
    }
}

/*
 * Frees the draw slots of the n parts of a non-chain object (MonsterFxPiece,
 * 0x40 bytes each).
 */
void MonsterFx_EndSeq(StageObj *obj, s16 partCount)
{
    u8 *bytes = obj->buf;
    s16 i;

    for (i = 0; i < partCount; i++) {
        if (*(s32 *) (bytes + 0x24) != 0) {
            DrawSlot_Free(*(s16 *) bytes);
        }
        bytes += 0x40;
    }
}

/* Frees the object (StageObj_Free). */
void MonsterFx_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Walks the 0xFF-ended list of monster kinds gMonsterFx_ModelSetKinds[k] and
 * returns the monster slot of the first one loaded (EmSlot_FindKind), or -1
 * when none is.
 */
s32 MonsterFx_FindModelSet(u8 listIdx)
{
    u8 *kinds = gStageObjK1C_ModelSets[listIdx];
    s16 slot;

    for (; *kinds != 0xFF; kinds++) {
        slot = EmSlot_FindKind(*kinds);
        if (slot != -1) {
            return slot;
        }
    }
    return -1;
}

/*
 * Draws one part (node is its DrawSlot): MonsterFx_DrawChain for the chain
 * types, MonsterFx_DrawSeq for the others.
 */
void MonsterFx_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    case 0:
    case 0xB:
        MonsterFx_DrawChain(node);
        break;
    default:
        MonsterFx_DrawSeq(node);
        break;
    }
}

/*
 * Draws one link with the owner's second model set (gScene.partModels of its
 * slot), sized by MonsterFx_GrowFactor. An attached breath stream draws its
 * first link as mesh 2 along its direction with a scrolling texture and the
 * others as mesh 0 facing the camera with a 4 x 4 flipbook; released links
 * draw mesh 1 facing the camera. Links further down are fainter.
 * MFX_SPARE_CHAIN draws meshes 4 and 9. Depth writes are off while drawing
 * (RS_ZWRITE).
 */
void MonsterFx_DrawChain(DrawSlot *node)
{
    f32 scale[3];
    f32 rot[3];
    f32 origin[3];
    f32 mtx[16];
    f32 uvMtx[16];
    StageObj *obj = node->owner;
    MonsterFxLink *link = (MonsterFxLink *) obj->buf + node->part;
    Model *modelSet;
    ModelMat *mats;
    ModelMatSet *model;
    u32 color;
    u16 mode = 0;
    s16 emSlot;
    s32 flipFrame;
    u8 alpha;
    f32 grow;

    emSlot = EmSlot_FindKind(((Em *) obj->owner)->type);
    if (emSlot != -1) {
        modelSet = gScene.partModels[emSlot];
        if (modelSet != NULL && modelSet->active != 0) {
            mats = modelSet->mats;
            grow = MonsterFx_GrowFactor(obj);
            scale[0] = grow * (link->sc[0] * link->scale);
            scale[1] = grow * (link->sc[1] * link->scale);
            scale[2] = grow * (link->sc[2] * link->scale);
            switch (obj->type) {
            case 0:
                rot[2] = (6.2831855f * (u32) link->rotZ) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                alpha = (((u32) link->color >> 24) & 0xFF) * (1.0f - 0.5f * (link->kind / 8.0f));
                color = ((u32) link->color & 0xFF) | (((((u32) link->color >> 8) & 0xFF) << 8) | ((alpha << 24) | ((((u32) link->color >> 16) & 0xFF) << 16)));
                if (link->releaseFrame < 0) {
                    mode |= 2;
                    switch (link->kind) {
                    case 0:
                        origin[0] = origin[1] = origin[2] = 0.0f;
                        rot[0] = (6.2831855f * Vec_Elevation(link->dir[0], origin)) / 65536.0f;
                        rot[1] = (6.2831855f * ((Vec_Heading(link->dir[0], origin) & 0xFFFF) + 0x4000)) / 65536.0f;
                        Mtx_RotateX(mtx, rot[0]);
                        Mtx_RotateY(mtx, rot[1]);
                        model = &modelSet->matSets[2];
                        Mtx_Translation(uvMtx, 0.0f, (link->frame & 0xF) / 16.0f, 0.0f);
                        break;
                    default:
                        Mtx_Concat3x3(mtx, gCamMatrix);
                        model = modelSet->matSets;
                        flipFrame = link->frame % 15;
                        Mtx_Translation(uvMtx, 0.25f * (flipFrame & 3), 0.25f * (flipFrame >> 2), 0.0f);
                        break;
                    }
                    Gs_SetState(0x19, (s32) uvMtx);
                } else {
                    Mtx_Concat3x3(mtx, gCamMatrix);
                    model = &modelSet->matSets[1];
                }
                break;
            case 11:
                rot[2] = (6.2831855f * (u32) link->rotZ) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                color = link->color;
                origin[0] = origin[1] = origin[2] = 0.0f;
                rot[0] = (6.2831855f * Vec_Elevation(link->dir[0], origin)) / 65536.0f;
                rot[1] = (6.2831855f * ((Vec_Heading(link->dir[0], origin) & 0xFFFF) + 0x4000)) / 65536.0f;
                switch (link->kind) {
                case 0:
                    Mtx_RotateX(mtx, rot[0]);
                    Mtx_RotateY(mtx, rot[1]);
                    model = &modelSet->matSets[4];
                    Mtx_Translation(uvMtx, 0.0f, (link->frame & 0xF) / 16.0f, 0.0f);
                    Gs_SetState(0x19, (s32) uvMtx);
                    mode |= 2;
                    break;
                default:
                    Mtx_RotateX(mtx, rot[0] + (6.2831855f * (u32) link->tiltX) / 65536.0f);
                    Mtx_RotateY(mtx, rot[1] + (6.2831855f * (u32) link->tiltY) / 65536.0f);
                    model = &modelSet->matSets[9];
                    break;
                }
                break;
            }
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, mtx, color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
    }
}

void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamYawMtx[];
extern u32 gStageObjK1C_DrawTbls[], gStageObjK1C_ModelSetIds[], MonsterFx_ChameleosImpactFrames[];

/*
 * Draws one part of a non-chain object. Its model is the type's entry for the
 * part's step (gMonsterFx_StepModels) in the stage's effect set or, with bit
 * 0x8000, in the set of the monster the type names (MonsterFx_FindModelSet).
 * By type and step it picks the rotation order, how the model faces the
 * camera (cm: 0 not, 1 fully, 2 about y only), the draw flags and a texture
 * scroll or flipbook (RS_UV_MTX); the dust types take the area's effect
 * colour (Effect_GetColor) with the part's alpha.
 */
void MonsterFx_DrawSeq(DrawSlot *node)
{
    u8 areaRed;
    u8 areaGreen;
    u8 areaBlue;
    u8 areaAlpha;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    u16 order;
    u16 mode;
    u8 faceCamera;
    StageObj *obj;
    MonsterFxPiece *piece;
    u32 color;
    Model *modelSet;
    u16 *stepModels;
    ModelMat *mats;
    ModelMatSet *model;
    f32 fade;
    s16 emSlot;
    u8 alpha;

    mode = 0;
    obj = node->owner;
    piece = (MonsterFxPiece *) obj->buf + node->part;
    modelSet = gStageModelSet;
    stepModels = ((u16 **) gStageObjK1C_DrawTbls)[obj->type];
    faceCamera = 1;
    if (stepModels[piece->idx] & 0x8000) {
        emSlot = MonsterFx_FindModelSet(((u8 (*)[2]) gStageObjK1C_ModelSetIds)[obj->type][0]);
        if (emSlot == -1) {
            return;
        }
        modelSet = gScene.partModels[emSlot];
    }
    if (modelSet != NULL && modelSet->active != 0) {
    model = &modelSet->matSets[stepModels[piece->idx] & ~0x8000];
    mats = modelSet->mats;
    color = piece->color;
    switch (obj->type) {
    case 1:
        switch (piece->idx) {
        case 0:
        default:
            mode |= 2;
            order = 3;
            break;
        case 1:
            mode |= 2;
            order = 0;
            break;
        case 2:
            mode |= 2;
            order = 6;
            Mtx_Translation(uvMtx, 0.0f, (piece->wait & 0x7F) / 128.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            faceCamera = 0;
            break;
        case 3:
            order = 3;
            mode |= 2;
            Mtx_Translation(uvMtx, 0.25f * ((piece->wait >> 1) & 3), 0.25f * (piece->wait >> 3), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 4:
            order = 3;
            break;
        }
        break;
    case 2:
        mode |= 2;
        switch (piece->idx) {
        default:
            order = 6;
            faceCamera = 0;
            break;
        case 1:
        case 2:
            order = 3;
            break;
        }
        break;
    case 3:
        mode |= 2;
        order = 7;
        faceCamera = 0;
        Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0x1F) / 32.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 4:
        mode |= 2;
        order = 3;
        break;
    case 5:
    case 6:
        switch (piece->idx) {
        case 0:
            order = 2;
            faceCamera = 0;
            break;
        default:
        case 1:
            order = 3;
            break;
        }
        break;
    case 7:
    case 13:
        Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        order = 3;
        if (obj->var1 == 0) {
            mode |= 2;
        }
        break;
    case 8:
        Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        switch (piece->idx) {
        case 0:
            faceCamera = 0;
            order = 7;
            break;
        default:
            order = 3;
            break;
        }
        break;
    case 9:
        Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        order = 3;
        switch (piece->idx) {
        case 1:
        case 2:
            mode |= 2;
            break;
        }
        break;
    case 10:
    case 18:
        Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        switch (piece->idx) {
        case 0:
        case 2:
            faceCamera = 0;
            order = 7;
            mode |= 2;
            break;
        }
        break;
    case 12:
        Effect_GetColor(gScene.area, &areaRed, &areaGreen, &areaBlue, &areaAlpha);
        fade = areaAlpha / 255.0f;
        switch (piece->idx) {
        case 0:
            faceCamera = 0;
            order = 7;
            mode |= 2;
            break;
        case 1:
            faceCamera = 0;
            order = 7;
            alpha = fade * ((piece->color >> 24) & 0xFF);
            color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
            break;
        default:
        case 2:
            order = 3;
            alpha = fade * ((piece->color >> 24) & 0xFF);
            color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
            break;
        }
        break;
    case 14:
        Effect_GetColor(gScene.area, &areaRed, &areaGreen, &areaBlue, &areaAlpha);
        alpha = (areaAlpha / 255.0f) * ((piece->color >> 24) & 0xFF);
        color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
        faceCamera = 2;
        switch (piece->idx) {
        case 0:
            order = 3;
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 1:
            order = 0;
            Mtx_Translation(uvMtx, 0.125f * (piece->sub & 3), 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        default:
        case 2:
        case 3:
            order = 3;
            break;
        }
        break;
    case 15:
    case 17:
        Effect_GetColor(gScene.area, &areaRed, &areaGreen, &areaBlue, &areaAlpha);
        alpha = (areaAlpha / 255.0f) * ((piece->color >> 24) & 0xFF);
        color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
        Mtx_Identity(uvMtx);
        switch (piece->idx) {
        case 0:
            faceCamera = 2;
            order = 3;
            break;
        case 1:
            faceCamera = 0;
            order = 2;
            break;
        case 2:
            faceCamera = 2;
            order = 0;
            Mtx_SetTrans(uvMtx, 0.125f * (piece->sub & 3), 0.0f, 0.0f);
            break;
        case 3:
            faceCamera = 0;
            order = 6;
            Mtx_SetTrans(uvMtx, 0.125f * (piece->sub & 3), 0.0f, 0.0f);
            break;
        case 4:
        case 5:
            faceCamera = 0;
            order = 6;
            break;
        case 6:
        case 7:
            order = 3;
            break;
        case 8:
        default:
            faceCamera = 0;
            order = 7;
            break;
        }
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 16:
        Effect_GetColor(gScene.area, &areaRed, &areaGreen, &areaBlue, &areaAlpha);
        alpha = (areaAlpha / 255.0f) * ((piece->color >> 24) & 0xFF);
        color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
        switch (piece->idx) {
        case 0:
            faceCamera = 0;
            order = 2;
            break;
        case 1:
            faceCamera = 1;
            order = 3;
            break;
        }
        Mtx_Identity(uvMtx);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 19:
        switch (piece->idx) {
        case 0:
            order = 2;
            faceCamera = 0;
            Effect_GetColor(gScene.area, &areaRed, &areaGreen, &areaBlue, &areaAlpha);
            alpha = (areaAlpha / 255.0f) * ((piece->color >> 24) & 0xFF);
            color = areaBlue | ((areaGreen << 8) | ((areaRed << 16) | (alpha << 24)));
            break;
        case 1:
        case 2:
            if (piece->sub == 0) {
                Mtx_Identity(uvMtx);
            } else {
                Mtx_Translation(uvMtx, 0.5f, 0.75f, 0.0f);
            }
            faceCamera = 1;
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 20:
    case 21:
        if (piece->sub == 0) {
            Mtx_Identity(uvMtx);
        } else {
            Mtx_Translation(uvMtx, 0.5f, 0.75f, 0.0f);
        }
        faceCamera = 1;
        order = 3;
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 22:
        faceCamera = 1;
        order = 3;
        break;
    case 23:
        faceCamera = 0;
        order = 2;
        Mtx_Identity(uvMtx);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 24:
        faceCamera = 0;
        order = 2;
        mode |= 2;
        break;
    case 25:
        if (obj->timer < 10) {
            fade = obj->timer / 10.0f;
        } else {
            fade = 1.0f;
        }
        alpha = fade * ((color >> 24) & 0xFF);
        color = (color & 0xFF) | ((((color >> 8) & 0xFF) << 8) | ((((color >> 16) & 0xFF) << 16) | (alpha << 24)));
        switch (piece->idx) {
        case 0:
            faceCamera = 0;
            order = 7;
            Mtx_Translation(uvMtx, 1.0f - (obj->timer & 0xFF) / 256.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 1:
            faceCamera = 1;
            order = 3;
            break;
        default:
            return;
        }
        mode |= 2;
        break;
    case 26:
        switch (piece->idx) {
        case 0:
            faceCamera = 1;
            order = 3;
            break;
        case 1:
        case 2:
            faceCamera = 0;
            order = 7;
            break;
        default:
            return;
        }
        mode |= 2;
        break;
    case 27:
        mode |= 2;
        faceCamera = 1;
        order = 3;
        Mtx_Identity(uvMtx);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 28:
        faceCamera = 1;
        order = 3;
        if (((color >> 24) & 0xFF) != 0xFF) {
            Mtx_Translation(uvMtx, 0.75f, 0.0f, 0.0f);
        } else {
            Mtx_Identity(uvMtx);
        }
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 29:
        switch (piece->idx) {
        case 0:
        case 1:
            faceCamera = 0;
            order = 7;
            mode |= 2;
            break;
        case 2:
            order = 3;
            faceCamera = 1;
            break;
        default:
        case 3:
            faceCamera = 0;
            order = 4;
            mode |= 2;
            Mtx_Translation(uvMtx, 0.0f, piece->idx / (f32) MonsterFx_TescaAuraFrames[piece->idx], 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        }
        break;
    case 30:
        order = 3;
        mode |= 4;
        if (piece->idx >= 4) {
            Mtx_Translation(uvMtx, 0.25f * (piece->sub & 3), 0.25f * (piece->sub / 4), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
        } else {
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
        }
        break;
    case 31:
        faceCamera = 1;
        order = 3;
        break;
    case 32:
        switch (piece->idx) {
        case 0:
            order = 2;
            faceCamera = 0;
            if (piece->wait <= 0) {
                Mtx_Identity(uvMtx);
            } else {
                /* w keeps p->wait as an s32 for the translation: so declared, the compiler loads
                 * the 0.0f arguments before the y argument, as the game's code does. */
                s32 w = piece->wait;

                Mtx_Translation(uvMtx, 0.0f, 0.125f * (w - 1) / (((u16 *) MonsterFx_ChameleosImpactFrames)[piece->idx] - 1), 0.0f);
            }
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 1:
            order = 2;
            faceCamera = 0;
            break;
        case 2:
            order = 3;
            faceCamera = 1;
            break;
        case 3:
            faceCamera = 2;
            break;
        default:
            return;
        }
        break;
    case 33:
        switch (piece->idx) {
        case 0:
            faceCamera = 1;
            order = 3;
            break;
        case 1:
            faceCamera = 0;
            order = 7;
            Mtx_Identity(uvMtx);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 2:
            faceCamera = 1;
            order = 3;
            mode |= 2;
            break;
        default:
            return;
        }
        break;
    case 34:
    case 35:
        faceCamera = 2;
        break;
    case 36:
        faceCamera = 0;
        order = 7;
        mode |= 2;
        break;
    case 37:
        mode |= 2;
        switch (piece->idx) {
        case 0:
            faceCamera = 1;
            break;
        case 1:
        case 2:
            faceCamera = 0;
            order = 4;
            break;
        default:
            return;
        }
        break;
    case 38:
        mode |= 2;
        switch (piece->idx) {
        case 0:
            faceCamera = 1;
            break;
        case 1:
        case 2:
            faceCamera = 0;
            order = 7;
            Mtx_Translation(uvMtx, 0.0f, 0.1875f * (piece->wait / 30.0f), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        default:
            return;
        }
        break;
    case 39:
        mode |= 2;
        order = 3;
        faceCamera = 1;
        break;
    case 40:
        mode |= 2;
        order = 6;
        faceCamera = 0;
        Mtx_Translation(uvMtx, 0.0f, 0.25f * (piece->wait / 8.0f), 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    }
    scale[0] = piece->sc[0] * piece->scale;
    scale[1] = piece->sc[1] * piece->scale;
    scale[2] = piece->sc[2] * piece->scale;
    rot[0] = (6.2831855f * (u32) (u16) piece->rotX) / 65536.0f;
    rot[1] = (6.2831855f * (u32) (u16) piece->rotY) / 65536.0f;
    rot[2] = (6.2831855f * (u32) (u16) piece->rotZ) / 65536.0f;
    /* Types 34 and 35 never set order: the game's code passes whatever the register holds, and this
     * code reads the unset variable the same way. */
    Mtx_FromScaleRotTrans(scale, rot, node->pos, order, mtx);
    switch (faceCamera) {
    case 0:
        break;
    case 1:
        Mtx_Concat3x3(mtx, gCamMatrix);
        break;
    case 2:
        Mtx_Concat3x3(mtx, gCamYawMtx);
        break;
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, mtx, color, mode, mats);
    Gs_SetState(0x6C, 1);
    }
}

/*
 * Places link w of a breath stream. The first link (kind 0) sits on the bone
 * matrix m and sets dir, the stream's direction (pitch o->angle2, the bone's
 * heading), with a point 150 ahead as its base. The others keep two (base,
 * direction) pairs from the link before (prev) and sit at the older pair's
 * base plus its direction times a reach: the second link uses this frame's
 * pair and a reach of 300 times the grow factor, the rest last frame's pair
 * and a reach growing to 100 over 15 frames. first is 0 at the set-up, which
 * fills both pairs.
 */
void MonsterFx_FollowBone(StageObj *obj, MonsterFxLink *w, MonsterFxLink *prev, f32 *boneMtx, f32 *dir, s16 first)
{
    f32 reach;
    s16 time;
    s16 i;
    f32 boneAxis[3];
    u16 bonePitch;
    u16 boneHeading;

    switch (w->kind) {
    case 0:
        Mtx_GetTrans(w->pos, boneMtx);
        Vec_Copy(boneAxis, boneMtx + 8);
        Vec_DirToAngles(boneAxis, (s16 *) &bonePitch, (s16 *) &boneHeading);
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 1.0f;
        Vec_RotateX(dir, (6.2831855f * obj->angle2) / 65536.0f);
        Vec_RotateY(dir, (6.2831855f * boneHeading) / 65536.0f);
        w->base[0][0] = w->pos[0] + 150.0f * dir[0];
        w->base[0][1] = w->pos[1] + 150.0f * dir[1];
        w->base[0][2] = w->pos[2] + 150.0f * dir[2];
        return;
    case 1:
        reach = 300.0f * MonsterFx_GrowFactor(obj);
        break;
    default:
        time = obj->timer;
        if (time < 0) {
            reach = 0.0f;
        } else if (time < 15) {
            reach = 100.0f * (time / 15.0f);
        } else {
            reach = 100.0f;
        }
        break;
    }
    if (first == 0) {
        for (i = 0; i < 2; i++) {
            Vec_Copy(w->dir[i], dir);
            Vec_Copy(w->base[i], prev->pos);
        }
    } else {
        if (w->kind == 1) {
            Vec_Copy(w->dir[0], dir);
            Vec_Copy(w->base[0], prev->pos);
        } else {
            for (i = 0; i < 1; i++) {
                Vec_Copy(w->dir[i], w->dir[i + 1]);
                Vec_Copy(w->base[i], w->base[i + 1]);
            }
        }
        Vec_Copy(w->dir[1], dir);
        Vec_Copy(w->base[1], prev->pos);
    }
    w->pos[0] = w->base[0][0] + w->dir[0][0] * reach;
    w->pos[1] = w->base[0][1] + w->dir[0][1] * reach;
    w->pos[2] = w->base[0][2] + w->dir[0][2] * reach;
}

/*
 * Gives a link a random z angle and a random z spin (-0x100 to 0xFF a frame).
 */
void MonsterFx_RandomSpin(MonsterFxLink *w)
{
    w->rotZ = Rand_Next(1);
    w->spinZ = (s16) ((Rand_Next(1) & 0xFFFF & 0x1FF) - 0x100);
}

/*
 * Moves a Kushala wind ring part to the next entry of
 * MonsterFx_KushalaRingKinds (o->var2 counts the entries): its step (0xFF:
 * the part is done), its quarter (sub) for step 2, and its frame counted from
 * the entry's start (MonsterFx_KushalaRingStarts) against o->var1.
 */
static void MonsterFx_NextKushalaRingStep(StageObj *obj, MonsterFxPiece *w)
{
    u8 k;

    k = D_008D6138[obj->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    if (w->idx == 2) {
        switch (obj->var2) {
        case 8:
        default:
            w->sub = 0;
            break;
        case 10:
            w->sub = 1;
            break;
        case 11:
            w->sub = 2;
            break;
        case 12:
            w->sub = 3;
            break;
        }
    }
    w->wait = obj->var1 - MonsterFx_KushalaRingStarts[obj->var2];
    obj->var2++;
}

/*
 * Starts a Kushala wind ring part's step on its first frame: 0 a tilted disc
 * on the owner's body, 1 a tilted disc at the ring's point facing o->angle2,
 * 2 (and others) a smaller flat puff heading out at its quarter (o->angle2 +
 * sub quarter turns). Each gets a random z angle and spins 60, 600 or 30
 * degrees over the step (MonsterFx_KushalaRingFrames), one way or the other.
 */
void MonsterFx_StartKushalaRingStep(StageObj *obj, MonsterFxPiece *w)
{
    f32 spinDegrees;

    switch (w->idx) {
    case 0:
        w->scale = obj->scale;
        w->flatScale = 0;
        w->rotX = (u16) (9102.0f * Rand_Unit()) + 0x31C7;
        spinDegrees = 60.0f;
        break;
    case 1:
        w->scale = obj->scale;
        w->flatScale = 0;
        w->rotX = (u16) (5461.0f * Rand_Unit()) + 0x38E4;
        spinDegrees = 600.0f;
        Vec_Copy(w->pos, obj->pos);
        w->rotY = obj->angle2;
        break;
    case 2:
    default:
        w->scale = 0.75f * obj->scale;
        w->flatScale = 1;
        spinDegrees = 30.0f;
        w->rotX = 0;
        Vec_Copy(w->pos, obj->pos);
        w->rotY = obj->angle2 + (w->sub << 14);
        w->speed = 120.0f * w->scale;
        break;
    }
    w->rotZ = Rand_Next(1);
    w->spinZ = 0.5f + 65536.0f * (spinDegrees / MonsterFx_KushalaRingFrames[w->idx]) / 360.0f;
    if (Rand_Next(1) & 1) {
        w->spinZ = -w->spinZ;
    }
}

/* Returns 1 when no part of the object has a step left (idx 0xFF), else 0. */
s32 MonsterFx_AllPartsDone(StageObj *obj)
{
    s16 partCount = gStageObjK1C_Counts[obj->type];
    u8 *bytes = obj->buf;
    s16 i;

    for (i = 0; i < partCount; i++) {
        if (bytes[2] != 0xFF) {
            return 0;
        }
        bytes += 0x40;
    }
    return 1;
}

/*
 * Restarts a Kushala wind ring: its entry counter back to 0 and its first
 * seven parts take their next steps (MonsterFx_NextKushalaRingStep).
 */
void MonsterFx_RestartKushalaRing(StageObj *obj)
{
    MonsterFxPiece *w = (MonsterFxPiece *) obj->buf;
    s16 i;

    obj->var2 = 0;
    for (i = 0; i < 7; i++, w++) {
        MonsterFx_NextKushalaRingStep(obj, w);
    }
}

/*
 * Moves a dust burst part (MFX_BREATH_DUST or MFX_IMPACT_DUST; other types
 * return) to the next entry of its type's kind list: 0xFF ends it; steps 4
 * and 6 become 4 or 5 and 6 or 7 at random, their quarter taken from the
 * entry number. Its frame counts from the entry's start (o->timer minus the
 * type's starts).
 */
void MonsterFx_NextDustBurstStep(StageObj *obj, MonsterFxPiece *w)
{
    u8 *kinds;
    u16 *starts;
    u8 k;

    switch (obj->type) {
    case 15:
        kinds = D_008D6738;
        starts = MonsterFx_BreathDustStarts;
        break;
    case 17:
        kinds = MonsterFx_ImpactDustKinds;
        starts = MonsterFx_ImpactDustStarts;
        break;
    default:
        return;
    }
    k = kinds[obj->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    switch (w->idx) {
    case 4:
        if (Rand_Next(1) & 1) {
            w->idx++;
        }
        switch (obj->var2) {
        case 4:
        default:
            w->sub = 0;
            break;
        case 5:
            w->sub = 1;
            break;
        case 6:
            w->sub = 2;
            break;
        case 7:
            w->sub = 3;
            break;
        }
        break;
    case 6:
        if (Rand_Next(1) & 1) {
            w->idx++;
        }
        switch (obj->var2) {
        case 8:
        default:
            w->sub = 0;
            break;
        case 9:
            w->sub = 1;
            break;
        case 10:
            w->sub = 2;
            break;
        case 11:
            w->sub = 3;
            break;
        }
        break;
    }
    w->wait = obj->timer - starts[obj->var2];
    obj->var2++;
}

/*
 * Starts a dust burst part's step at the object's point: flat discs (0; 2
 * with a random texture column), a turning cloud (1), a falling puff (3),
 * tumbling chunks thrown up and out toward their quarter (4, 5), puffs
 * spreading low along their quarter (6, 7) or a tilted spinning disc (8).
 */
void MonsterFx_StartDustBurstStep(StageObj *obj, MonsterFxPiece *w)
{
    w->rotX = 0;
    w->rotY = 0;
    w->rotZ = 0;
    w->spinX = 0;
    w->spinY = 0;
    w->spinZ = 0;
    Vec_Copy(w->pos, obj->pos);
    switch (w->idx) {
    case 0:
        w->flatScale = 1;
        w->rotZ = 0x4000;
        break;
    case 1:
        w->flatScale = 0;
        w->rotY = Rand_Next(1);
        w->spinY = 0.5f + 65536.0f * (48.0f / MonsterFx_DustBurstFrames[w->idx]) / 360.0f;
        break;
    case 2:
        w->flatScale = 1;
        w->rotZ = 0x4000;
        w->sub = Rand_Next(1) & 3;
        break;
    case 3:
        w->flatScale = 0;
        w->rotY = Rand_Next(1);
        w->speed = 5.0f * w->scale;
        break;
    case 4:
    case 5:
        w->flatScale = 0;
        w->rotX = Rand_Next(1);
        w->rotY = Rand_Next(1);
        w->rotZ = Rand_Next(1);
        w->spinX = (Rand_Next(1) & 0x1FF) - 0x100;
        w->spinY = (Rand_Next(1) & 0x1FF) - 0x100;
        w->spinZ = (Rand_Next(1) & 0x1FF) - 0x100;
        w->speed = w->scale * ((400.0f + 200.0f * Rand_Unit()) / MonsterFx_DustBurstFrames[w->idx]);
        w->riseSpeed = w->scale * (70.0f + 25.0f * Rand_Unit());
        break;
    case 6:
    case 7:
        w->flatScale = 1;
        w->rotZ = Rand_Next(1);
        w->spinZ = 0.5f + 65536.0f * (50.0f / MonsterFx_DustBurstFrames[w->idx]) / 360.0f;
        if (Rand_Next(1) & 1) {
            w->spinZ = -w->spinZ;
        }
        w->rotY = obj->angle + (w->sub << 14);
        w->speed = 30.0f * w->scale;
        break;
    case 8:
    default:
        w->flatScale = 0;
        w->rotX = (u16) (5461.0f * Rand_Unit()) + 0x38E4;
        w->rotZ = Rand_Next(1);
        w->spinZ = 0.5f + 65536.0f * (600.0f / MonsterFx_DustBurstFrames[w->idx]) / 360.0f;
        if (Rand_Next(1) & 1) {
            w->spinZ = -w->spinZ;
        }
        break;
    }
}

/*
 * Returns the size factor of a chain's links: for MFX_BREATH_STREAM 0.2
 * growing to 1.0 over its first 15 frames; 1.0 for the other types.
 */
f32 MonsterFx_GrowFactor(StageObj *obj)
{
    f32 factor;
    s16 time;

    switch (obj->type) {
    default:
        return 1.0f;
    case 0:
        time = obj->timer;
        if (time < 0) {
            return 0.2f;
        }
        if (time < 15) {
            factor = 0.2f + 0.8f * time / 15.0f;
        } else {
            factor = 1.0f;
        }
        return factor;
    }
}

/* Built at optimization level 3: at level 4 the compiler swaps the registers of the index and of
 * the table's address, unlike the game's code. */
#pragma optimization_level 3
/*
 * Moves a dust kick part to the next entry of MonsterFx_DustKickKinds (0xFF:
 * done), its frame counted from the entry's start (o->timer minus
 * MonsterFx_DustKickStarts). Steps 1 and 2 head behind the object (o->angle +
 * half a turn), spread by the entry number and a random angle; others face
 * o->angle.
 */
void MonsterFx_NextDustKickStep(StageObj *obj, MonsterFxPiece *w)
{
    u16 *starts = MonsterFx_DustKickStarts;
    u8 *kinds;
    u8 k;
    s32 behindYaw;

    kinds = MonsterFx_DustKickKinds;
    k = *(kinds += obj->var2);
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    w->wait = obj->timer - starts[obj->var2];
    switch (w->idx) {
    case 1:
        behindYaw = obj->angle + 0x7FFF + 1 + (obj->var2 % 3 - 1) * 0x18E4;
        w->rotY = behindYaw + ((Rand_Next(1) & 0x1FFF) - 0x1000);
        break;
    case 2:
        behindYaw = obj->angle + 0x7FFF + 1 + ((obj->var2 & 1) - 1) * 0x2000;
        w->rotY = behindYaw + (Rand_Next(1) & 0x1FFF);
        break;
    default:
        w->rotY = obj->angle;
        break;
    }
    obj->var2++;
}
#pragma optimization_level reset

/*
 * Starts a dust kick part's step at the object's point: 0 a cloud that keeps
 * its own height (the floor); 1 and 2 flat spinning puffs with a random
 * texture, moving out at 20 (step 2: half size, 15) times the size.
 */
void MonsterFx_StartDustKickStep(StageObj *obj, MonsterFxPiece *w)
{
    f32 keptY = w->pos[1];

    Vec_Copy(w->pos, obj->pos);
    switch (w->idx) {
    case 0:
        w->flatScale = 0;
        w->scale = obj->scale;
        w->pos[1] = keptY;
        break;
    case 1:
        w->flatScale = 1;
        w->rotZ = Rand_Next(1);
        w->spinZ = 0x5B;
        w->sub = Rand_Next(1) & 1;
        w->scale = obj->scale;
        w->speed = 20.0f * w->scale;
        break;
    case 2:
        w->flatScale = 1;
        w->rotZ = Rand_Next(1);
        w->spinZ = 0x5B;
        w->sub = Rand_Next(1) & 1;
        w->scale = 0.5f * obj->scale;
        w->speed = 15.0f * w->scale;
        break;
    }
}

/*
 * Places the object 200 behind and 50 below the owner's bone 0xE (in the
 * bone's frame), facing away from the owner's heading.
 */
void MonsterFx_PlaceBehindOwner(StageObj *obj)
{
    f32 *boneMtx;
    Em *em;
    f32 offset[3];

    em = obj->owner;
    boneMtx = (f32 *)Em_Bone(em, 0xE);
    offset[0] = 0.0f;
    offset[1] = -50.0f;
    offset[2] = -200.0f;
    Vec_RotateSelf(offset, boneMtx);
    obj->pos[0] = boneMtx[12] + offset[0];
    obj->pos[1] = boneMtx[13] + offset[1];
    obj->pos[2] = boneMtx[14] + offset[2];
    obj->angle = em->rotY + 0x7FFF + 1;
}

/*
 * Starts a Lunastra/Teostra aura part's step: 0 a cloud at random angles, 1 a
 * tilted disc facing o->angle2, 2 a puff scattered around the object's point
 * (up to 300, 150 and 400 times the size, turned by o->angle2) with the
 * owner's sound 0x18 for every other one (o->angle counts them), 3 a larger
 * tilted disc rising from the point. Spins and speeds are spread over the
 * step (MonsterFx_TescaAuraFrames).
 */
void MonsterFx_StartTescaAuraStep(StageObj *obj, MonsterFxPiece *w)
{
    Em *em = obj->owner;
    f32 offset[3];

    w->rotX = 0;
    w->rotY = 0;
    w->rotZ = 0;
    w->spinX = 0;
    w->spinY = 0;
    w->spinZ = 0;
    w->scale = obj->scale;
    switch (w->idx) {
    case 0:
        w->rotX = Rand_Next(1);
        w->rotY = Rand_Next(1);
        w->rotZ = Rand_Next(1);
        w->spinZ = 0.5f + (65536.0f * (16.0f / MonsterFx_TescaAuraFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->spinZ = -w->spinZ;
        }
        w->flatScale = 0;
        break;
    case 1:
        w->rotX = (s16) (0.5f + (65536.0f * (70.0f + 50.0f * Rand_Unit())) / 360.0f);
        w->rotY = obj->angle2;
        w->rotZ = Rand_Next(1);
        w->flatScale = 0;
        w->spinZ = 0.5f + (65536.0f * (60.0f / MonsterFx_TescaAuraFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->spinZ = -w->spinZ;
        }
        break;
    case 2:
        w->flatScale = 1;
        w->rotZ = Rand_Next(1);
        w->spinZ = 0.5f + (65536.0f * (10.0f / MonsterFx_TescaAuraFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->spinZ = -w->spinZ;
        }
        Vec_Copy(w->pos, obj->pos);
        offset[0] = w->scale * (300.0f * Rand_Signed());
        offset[1] = w->scale * (150.0f * Rand_Signed());
        offset[2] = w->scale * (400.0f * Rand_Signed());
        Vec_RotateY(offset, (6.2831855f * obj->angle2) / 65536.0f);
        w->pos[0] += offset[0];
        w->pos[1] += offset[1];
        w->pos[2] += offset[2];
        w->speed = (100.0f + 100.0f * Rand_Unit()) / MonsterFx_TescaAuraFrames[w->idx];
        if (!(obj->angle & 1)) {
            Em_PlaySeChecked((struct Em *)em, 0x18, 0, w->pos, 6, 0);
        }
        obj->angle++;
        break;
    case 3:
    default:
        w->rotX = (s16) (0.5f + (65536.0f * (70.0f + 50.0f * Rand_Unit())) / 360.0f);
        w->rotY = Rand_Next(1);
        if (!(Rand_Next(1) & 1)) {
            w->spinY = -w->spinY;
        }
        w->flatScale = 0;
        w->scale *= 1.3f;
        Vec_Copy(w->pos, obj->pos);
        w->speed = (200.0f + 100.0f * Rand_Unit()) / MonsterFx_TescaAuraFrames[w->idx];
        break;
    }
}

/*
 * Moves a Lunastra/Teostra aura part to its next step: a part in step 0
 * restarts its frame; the others take the next entry of
 * MonsterFx_TescaAuraKinds (0xFF: done), their frame counted from the entry's
 * start against o->var1.
 */
static void MonsterFx_NextTescaAuraStep(StageObj *obj, MonsterFxPiece *w)
{
    u8 k;

    if (w->idx == 0) {
        w->wait = 1;
        return;
    }
    k = MonsterFx_TescaAuraKinds[obj->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    w->wait = obj->var1 - MonsterFx_TescaAuraStarts[obj->var2];
    obj->var2++;
}

/*
 * Restarts the bursts of a Lunastra/Teostra aura: its entry counter back to 2
 * and parts 2 to 7 take their next steps; parts 0 and 1, the aura on the
 * body, keep running.
 */
void MonsterFx_RestartTescaAuraBursts(StageObj *obj)
{
    MonsterFxPiece *w = (MonsterFxPiece *) obj->buf + 2;
    s16 i;

    obj->var2 = 2;
    for (i = 2; i < 8; i++, w++) {
        MonsterFx_NextTescaAuraStep(obj, w);
    }
}

/*
 * Returns 1 when the parts from 2 on (the aura's bursts) have no step left,
 * else 0.
 */
s32 MonsterFx_TescaAuraBurstsDone(StageObj *obj)
{
    MonsterFxPiece *w = (MonsterFxPiece *) obj->buf + 2;
    s16 partCount = gStageObjK1C_Counts[obj->type];
    s16 i;

    for (i = 2; i < partCount; i++, w++) {
        if (w->idx != 0xFF) {
            return 0;
        }
    }
    return 1;
}

/*
 * Writes to out the position of the owner's bone that o->var2 picks: 0x1E for
 * 1, 0x19 for 2, 0x15 for the others (MFX_FATALIS_BONE).
 */
void MonsterFx_GetOwnerBone(StageObj *obj, f32 *out)
{
    Em *em = obj->owner;

    switch (obj->var2) {
    default:
        Em_BonePos(em, 0x15, out);
        break;
    case 1:
        Em_BonePos(em, 0x1E, out);
        break;
    case 2:
        Em_BonePos(em, 0x19, out);
        break;
    }
}

/*
 * Returns 1 while the owning monster is active and its stateVar1 is not 3,
 * else 0.
 */
s32 MonsterFx_OwnerActive(Em *em)
{
    if (em->active == 0 || em->stateVar1 == 3) {
        return 0;
    }
    return 1;
}

/* Moves part w by up to r at random along each axis. */
void MonsterFx_Jitter(MonsterFxPiece *w, f32 radius)
{
    w->pos[0] += radius * Rand_Signed();
    w->pos[1] += radius * Rand_Signed();
    w->pos[2] += radius * Rand_Signed();
}

/*
 * Writes to out count distinct random numbers below n (n at most 4): each
 * number drawn below n - i is pushed past the ones already taken. MonsterFx
 * uses it to send parts to different quarters.
 */
void StageObj_RandomDistinct(s16 range, s16 count, s16 *out)
{
    s16 used[4];
    s16 j;
    s16 k;
    s16 i;

    if (range > 4) {
        range = 4;
    }
    for (i = 0; i < count; i++) {
        out[i] = Rand_Next(1) % (range - i);
        for (j = 0; j < i; j++) {
            used[j] = 0;
        }
        while (1) {
            k = 0;
            for (j = 0; j < i; j++) {
                if (used[j] == 0 && out[j] <= out[i]) {
                    used[j] = 1;
                    k++;
                }
            }
            if (k <= 0) {
                break;
            }
            out[i] += k;
        }
    }
}

/*
 * Allocates a stage object with the blocks MonsterFx type k needs (2 for the
 * chains and MFX_BLAST, 1 for the others); returns it, or NULL when none is
 * free.
 */
StageObj *MonsterFx_Alloc(u8 fxType)
{
    s16 blocks;

    switch (fxType) {
    case 0:
    case 0xB:
        blocks = 2;
        break;
    case 1:
        blocks = 2;
        break;
    case 2:
    case 3:
    case 4:
    default:
        blocks = 1;
        break;
    }
    return StageObj_Alloc(blocks);
}

/*
 * Creates a MonsterFx object of type k on the monster em with size scale and
 * returns it, or NULL. Only the two auras (MFX_KUSHALA_WIND_RING,
 * MFX_TESCA_AURA) are made while the monster is out of the current area.
 */
StageObj *MonsterFx_CreateOnEm(Em *em, u8 fxType, f32 scale)
{
    StageObj *obj;

    if (fxType != 0xC && fxType != 0x1D && !((s32)Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    obj = MonsterFx_Alloc(fxType);
    if (obj == NULL) {
        return NULL;
    }
    obj->kind = STAGEOBJ_KIND_MONSTER_FX;
    obj->update = MonsterFx_Update;
    obj->owner = em;
    obj->type = fxType;
    obj->scale = scale;
    return obj;
}

/*
 * As MonsterFx_CreateOnEm, and keeps the variant c in var2 (what Congalala
 * ate, the Hermitaur drops' gravity, the Fatalis bone).
 */
void MonsterFx_CreateOnEmVariant(Em *em, u8 fxType, u8 variant, f32 scale)
{
    StageObj *obj = MonsterFx_CreateOnEm(em, fxType, scale);

    if (obj != NULL) {
        obj->var2 = variant;
    }
}

/*
 * Creates a MonsterFx object of type k, with no owner, at the point src with
 * size f.
 */
void MonsterFx_CreateAt(f32 *point, u8 fxType, f32 size)
{
    StageObj *obj = MonsterFx_Alloc(fxType);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_MONSTER_FX;
        obj->update = MonsterFx_Update;
        obj->type = fxType;
        obj->scale = size;
        Vec_Copy(obj->pos, point);
    }
}

/*
 * Creates a MonsterFx object of type k, with no owner, at pos and moving
 * along the direction pos2 (kept as its first part's position until the set-
 * up reads it); used for MFX_BREATH_MOTE.
 */
void MonsterFx_CreateMoving(f32 *pos, f32 *dir, u8 fxType, f32 scale)
{
    StageObj *obj;

    if ((obj = MonsterFx_Alloc(fxType)) != NULL) {
        obj->kind = STAGEOBJ_KIND_MONSTER_FX;
        obj->update = MonsterFx_Update;
        obj->type = fxType;
        obj->scale = scale;
        Vec_Copy(obj->pos, pos);
        Vec_Copy((f32 *) (obj->buf + 4), dir);
    }
}

/*
 * Creates a MonsterFx object of type k, with no owner, at pos facing the
 * heading v; returns it or NULL.
 */
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 yaw, u8 fxType, f32 scale)
{
    StageObj *obj;

    if ((obj = MonsterFx_Alloc(fxType)) == NULL) {
        return NULL;
    }
    obj->kind = STAGEOBJ_KIND_MONSTER_FX;
    obj->update = MonsterFx_Update;
    obj->type = fxType;
    obj->scale = scale;
    obj->angle = yaw;
    Vec_Copy(obj->pos, pos);
    return obj;
}

/* As MonsterFx_CreateAtYaw, with the pitch w in angle2. */
void MonsterFx_CreateAtAngled(f32 *pos, u16 pitch, u16 yaw, u8 fxType, f32 scale)
{
    StageObj *obj = MonsterFx_CreateAtYaw(pos, yaw, fxType, scale);

    if (obj != NULL) {
        obj->angle2 = pitch;
    }
}

/*
 * Creates a MonsterFx object of type k on the monster em, with the pitch w
 * (angle2) and the heading v (angle; types 10 and 18 add the monster's
 * heading to it), when the monster is in the current area.
 */
void MonsterFx_CreateOnEmAngled(Em *em, u8 fxType, u16 pitch, s16 yawOffset, f32 scale)
{
    StageObj *obj;

    if ((s32)Player_InCurrentArea(em) & 0xFF) {
        obj = MonsterFx_Alloc(fxType);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_MONSTER_FX;
            obj->update = MonsterFx_Update;
            obj->owner = em;
            obj->type = fxType;
            obj->scale = scale;
            obj->angle2 = pitch;
            obj->angle = yawOffset;
        }
    }
}

/*
 * The AreaFlyer update: switches on the state to its Setup, Run, End and
 * Free.
 */
void AreaFlyer_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        AreaFlyer_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        AreaFlyer_Run(obj);
        break;
    case STAGEOBJ_END:
        AreaFlyer_End(obj);
        break;
    case STAGEOBJ_FREE:
        AreaFlyer_Free(obj);
        break;
    }
}

/*
 * Sets the flyer up: gAreaState.flyerPos at its area point (type), size 0.7
 * and a draw slot (with none, gAreaState.flyer is cleared); then runs its
 * first frame.
 */
void AreaFlyer_Setup(StageObj *obj)
{
    AreaState *area = &gAreaState;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    obj->angle = 0;
    obj->ownerIndex = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    AreaFlyer_GetCenter(gAreaState.flyerPos, obj->type);
    obj->scale = 0.7f;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = AreaFlyer_Draw;
    } else {
        area->flyer = 0;
        obj->node = NULL;
    }
    AreaFlyer_Run(obj);
}

/*
 * Moves the flyer on a loop of 1400 around its area point (sines of 3, 4 and
 * 5 times its phase, angle) into gAreaState.flyerPos, spins it (timer) and
 * steps its wing frame (ownerIndex, 0 to 5). While the local player's
 * actDetectTimer runs it flashes: var0 counts 13 frames, then var1 13 more,
 * and again. Its draw slot goes into the 32-entry draw table.
 */
void AreaFlyer_Run(StageObj *obj)
{
    AreaState *area = &gAreaState;
    f32 *base;
    f32 phase;

    base = gAreaFlyerCenters[obj->type];
    obj->timer += 0x12;
    obj->angle += 5;
    obj->ownerIndex++;
    if (obj->ownerIndex > 5) {
        obj->ownerIndex = 0;
    }
    phase = (6.2831855f * obj->angle) / 65536.0f;
    area->flyerPos[0] = base[0] + 1400.0f * Math_Sin(3.0f * phase);
    area->flyerPos[1] = base[1] + 1400.0f * Math_Sin(4.0f * phase);
    area->flyerPos[2] = base[2] + 1400.0f * Math_Sin(5.0f * phase);
    if (obj->var0 == 0) {
        if ((u16) gPlayers[gScene.localPlayer].actDetectTimer != 0) {
            obj->var0 = 1;
        }
    } else {
        obj->var0++;
        if (obj->var0 > 0xD) {
            if (++obj->var1 > 0xD) {
                obj->var0 = 0;
                obj->var1 = 0;
            }
        }
    }
    if (obj->node != NULL) {
        obj->node->pos[0] = area->flyerPos[0];
        obj->node->pos[1] = area->flyerPos[1];
        obj->node->pos[2] = area->flyerPos[2];
        OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
    }
}

/* Frees the draw slot and moves the object to its free state. */
void AreaFlyer_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* Frees the object (StageObj_Free). */
void AreaFlyer_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws the flyer unless a scene event runs: common mesh 150, spinning about
 * y, with only the material of its wing frame shown. In the first 13 frames
 * of a flash it also draws mesh 91 400 toward the camera, facing it, with its
 * scale and colour keys and depth writes off.
 */
void AreaFlyer_Draw(DrawSlot *node)
{
    StageObj *obj;
    ModelMat *mats;
    Model *mdl;
    ModelMatSet *mesh;
    ModelMat *meshMat;
    s16 i;
    s32 color;
    f32 scale[3];
    f32 rot[3];
    f32 dir[3];
    f32 mtx[16];
    f32 uvMtx[16];

    mdl = gCommonModels[0];
    obj = node->owner;
    if (gScene.eventRunning == 0 && mdl != NULL && mdl->active != 0 && obj->visible != 0) {
        mats = mdl->mats;
        color = -1;
        mesh = &mdl->matSets[150];
        scale[0] = obj->scale;
        scale[1] = obj->scale;
        scale[2] = obj->scale;
        rot[0] = 0.0f;
        rot[1] = (6.2831855f * (u16) obj->timer) / 65536.0f;
        rot[2] = 0.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, mtx);
        for (i = 1; i < mesh->count; i++) {
            meshMat = &mats[mesh->mat[i]];
            if (i == obj->ownerIndex / 2 + 1) {
                meshMat->alpha = 1.0f;
            } else {
                meshMat->alpha = 0.0f;
            }
        }
        Effect_DrawModel(mesh, mtx, color, 0, mats);
        if (obj->var0 != 0 && obj->var1 == 0) {
            Gs_SetState(0x6C, 0);
            Mtx_Identity(uvMtx);
            Gs_SetState(0x19, (s32) uvMtx);
            Key_LerpVec2(gAreaFlyerScaleKeys, scale, obj->var0);
            Color_EvalKeys((s32 *)gAreaFlyerColorKeys, obj->var0, (u32 *)&color);
            dir[0] = gCamMatrix[12] - node->pos[0];
            dir[1] = gCamMatrix[13] - node->pos[1];
            dir[2] = gCamMatrix[14] - node->pos[2];
            Vec_Normalize(dir);
            dir[0] *= 400.0f;
            dir[1] *= 400.0f;
            dir[2] *= 400.0f;
            dir[0] += node->pos[0];
            dir[1] += node->pos[1];
            dir[2] += node->pos[2];
            Mtx_FromScaleRotTrans(scale, rot, dir, 0, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Effect_DrawModel(&mdl->matSets[91], mtx, color, 2, mats);
            Gs_SetState(0x6C, 1);
        }
    }
}

/* Copies area point idx (gAreaFlyerCenters) to out. */
void AreaFlyer_GetCenter(f32 *out, u8 idx)
{
    Vec_Copy(out, gAreaFlyerCenters[idx]);
}

/*
 * Creates the flyer of area point k when the quest has one (gQuest.flyer),
 * with no blocks; sets gAreaState.flyer while it exists. Area_StartObjects
 * calls it with the area's point.
 */
void AreaFlyer_Create(u8 point)
{
    AreaState *area = &gAreaState;
    StageObj *obj;

    if (gQuest.flyer != 0) {
        obj = StageObj_Alloc(0);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_AREA_FLYER;
            obj->update = AreaFlyer_Update;
            obj->type = point;
            area->flyer = 1;
        } else {
            area->flyer = 0;
        }
    }
}

/*
 * The GunFx update: switches on the state to GunFx_Setup, GunFx_Run,
 * GunFx_End and GunFx_Free.
 */
void GunFx_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        GunFx_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        GunFx_Run(obj);
        break;
    case STAGEOBJ_END:
        GunFx_End(obj);
        break;
    case STAGEOBJ_FREE:
        GunFx_Free(obj);
        break;
    }
}

/* a part of a GunFx object (0x20 bytes, in its work block); GunFx_Draw reads the same layout */
typedef struct GunFxPart {
    s16 slot;                   /* 0x00 */
    s8 t;                       /* 0x02: its frame, negative: not shown yet */
    s8 frame;                   /* 0x03: a texture frame (4 x 4) */
    s32 color;                  /* 0x04 */
    f32 sc[3];                  /* 0x08: its scale */
    DrawSlot *node;                /* 0x14 */
    f32 size;                  /* 0x18: a size */
    s16 spin;                  /* 0x1C: its spin */
    s16 angle;                  /* 0x1E: its z angle */
} GunFxPart;

/* the work block of a GunFx object (its blocks of the object buffer) */
typedef struct GunFxWork {
    f32 m[16];                  /* 0x000 */
    f32 scale;                  /* 0x040 */
    GunFxPart part[11];         /* 0x044 */
    u8 count;                   /* 0x1A4: parts in use */
    s8 caseSideSpeed;                  /* 0x1A5 */
    s8 caseRiseSpeed;                  /* 0x1A6 */
    u8 shellsDone;                  /* 0x1A7 */
    u8 shellPhase[3];               /* 0x1A8: type 3 */
    u8 shellBounces[3];               /* 0x1AB */
    s8 shellDelay[3];               /* 0x1AE */
    s8 shellSideSpeed[3];               /* 0x1B1 */
    s8 shellRiseSpeed[3];               /* 0x1B4 */
    u8 sprayLevel;              /* 0x1B7: the row of the spray tables (stream lengths, spreads) */
    s32 angle[3];               /* 0x1B8: types 2 and 3 */
    s16 slot[3];                /* 0x1C4: type 3 */
    u8 unk1CA[2];
    DrawSlot *node[3];             /* 0x1CC */
} GunFxWork;

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Scale(f32 *out, f32 *v, f32 s);
void GunFx_Draw(DrawSlot *node);

extern u32 GunFx_SmokeOffsetsLow[], GunFx_SmokeOffsetsHigh[], GunFx_SmokeDelaysLow[], GunFx_SmokeDelaysHigh[], GunFx_SmokeSizesLow[], GunFx_SmokeSizesHigh[], GunFx_ChargeDelays[], GunFx_ChargeTipSizes[], GunFx_SprayStreamLengths[], GunFx_SprayStreamSpreads[], GunFx_SprayCloudSpreads[], GunFx_SprayDropLengths[], GunFx_SprayStreamStarts[], GunFx_BarrelSmokeDelays[], GunFx_BarrelSmokeSizes[];
/*
 * Sets a GunFx object up from the owner's bone 0xE (copied to w->m; likely
 * the weapon) and the muzzle, 105, 15, 167 in that bone's frame: the shelling
 * flash (3 slots) and smoke (3, 9, 6 or 11 parts by shell level, from the
 * level's offsets, delays and sizes), the spent bowgun case and the three
 * gunlance shells (speeds and delays), the wyvern charge (8 parts along the
 * barrel), the Hermitaur spray (4 parts at its bone 0x1F) or the barrel smoke
 * (4 parts). Each part gets a draw slot.
 */
void GunFx_Setup(StageObj *obj)
{
    GunFxWork *w;
    GunFxPart *piece;
    u8 i;
    u8 j;
    f32 (*pos)[3];
    s32 *delays;
    f32 *sizes;
    f32 *spread;
    f32 jitter;
    f32 offset[3];
    f32 partOffset[3];
    f32 pullBack[3];
    f32 randOffsets[3][3];

    w = (GunFxWork *) obj->buf;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->timer = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    piece = w->part;
    Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0xE)));
    switch (obj->type) {
    case 0:
        w->count = 3;
        Mtx_GetTrans(obj->pos, w->m);
        w->scale = obj->scale;
        for (i = 0; i < w->count; i++, piece++) {
            piece->slot = DrawSlot_Alloc();
            if (piece->slot != -1) {
                piece->node = DrawSlot_Get(piece->slot);
                piece->node->owner = obj;
                piece->node->part = i;
                piece->node->draw = GunFx_Draw;
            } else {
                piece->node = NULL;
            }
        }
        break;
    case 1:
        switch (obj->var2) {
        case 0:
            w->count = 3;
            pos = (f32 (*)[3]) GunFx_SmokeOffsetsLow[0];
            delays = (s32 *) GunFx_SmokeDelaysLow[0];
            sizes = (f32 *) GunFx_SmokeSizesLow[0];
            break;
        case 1:
            w->count = 9;
            pos = (f32 (*)[3]) GunFx_SmokeOffsetsLow[1];
            delays = (s32 *) GunFx_SmokeDelaysLow[1];
            sizes = (f32 *) GunFx_SmokeSizesLow[1];
            break;
        case 2:
            w->count = 6;
            spread = (f32 *) GunFx_SmokeOffsetsHigh[0];
            for (j = 0; j < 3; j++) {
                randOffsets[j][0] = (f32) (u32) (1.0f + 2.0f * spread[0]) * Rand_Unit() - 150.0f;
                randOffsets[j][1] = (f32) (u32) (1.0f + 2.0f * spread[1]) * Rand_Unit() - 150.0f;
                randOffsets[j][2] = (f32) (u32) (1.0f + 2.0f * spread[2]) * Rand_Unit() - 150.0f;
            }
            pos = randOffsets;
            delays = (s32 *) GunFx_SmokeDelaysHigh[0];
            sizes = (f32 *) GunFx_SmokeSizesHigh[0];
            break;
        case 3:
            w->count = 11;
            pos = (f32 (*)[3]) GunFx_SmokeOffsetsHigh[1];
            delays = (s32 *) GunFx_SmokeDelaysHigh[1];
            sizes = (f32 *) GunFx_SmokeSizesHigh[1];
            break;
        }
        for (i = 0; i < w->count; i++, piece++) {
            piece->sc[2] = 0.0f;
            switch (i) {
            case 0:
            case 1:
            case 2:
                jitter = 25.0f * Rand_Signed();
                partOffset[0] = jitter + pos[i][0];
                partOffset[1] = jitter + pos[i][1];
                partOffset[2] = jitter + pos[i][2];
                Vec_RotateY(partOffset, 0.5462881f);
                Vec_RotateSelf(partOffset, w->m);
                offset[0] = 105.0f;
                offset[1] = 15.0f;
                offset[2] = 167.0f;
                Vec_RotateSelf(offset, w->m);
                offset[0] += partOffset[0];
                offset[1] += partOffset[1];
                offset[2] += partOffset[2];
                Mtx_GetTrans(obj->pos, w->m);
                piece->t = delays[i];
                piece->angle = Rand_Next(1);
                piece->spin = 0.5f + 65536.0f * ((s16) (s32) (30.0f * Rand_Signed()) / 30.0f) / 360.0f;
                piece->size = sizes[i];
                break;
            case 3:
            case 4:
            case 5:
            case 9:
                partOffset[0] = pos[i - 3][0];
                partOffset[1] = pos[i - 3][1];
                partOffset[2] = pos[i - 3][2];
                Vec_RotateY(partOffset, 0.5462881f);
                Vec_RotateSelf(partOffset, w->m);
                offset[0] = 105.0f;
                offset[1] = 15.0f;
                offset[2] = 167.0f;
                Vec_RotateSelf(offset, w->m);
                offset[0] += partOffset[0];
                offset[1] += partOffset[1];
                offset[2] += partOffset[2];
                Mtx_GetTrans(obj->pos, w->m);
                piece->t = delays[i];
                piece->size = sizes[i - 3];
                piece->frame = -1;
                piece->angle = 0;
                piece->spin = 0;
                break;
            case 6:
            case 7:
            case 8:
            case 10:
                partOffset[0] = pos[i - 6][0];
                partOffset[1] = pos[i - 6][1];
                partOffset[2] = pos[i - 6][2];
                Vec_RotateY(partOffset, 0.5462881f);
                Vec_RotateSelf(partOffset, w->m);
                offset[0] = 105.0f;
                offset[1] = 15.0f;
                offset[2] = 167.0f;
                Vec_RotateSelf(offset, w->m);
                offset[0] += partOffset[0];
                offset[1] += partOffset[1];
                offset[2] += partOffset[2];
                Mtx_GetTrans(obj->pos, w->m);
                piece->t = delays[i];
                piece->size = sizes[i - 6];
                piece->angle = Rand_Next(1);
                piece->spin = 0;
                piece->frame = 0;
                break;
            }
            piece->slot = DrawSlot_Alloc();
            if (piece->slot != -1) {
                piece->node = DrawSlot_Get(piece->slot);
                piece->node->owner = obj;
                piece->node->part = i;
                piece->node->draw = GunFx_Draw;
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
            } else {
                piece->node = NULL;
            }
        }
        break;
    case 2:
        Mtx_GetTrans(obj->pos, w->m);
        w->caseSideSpeed = -5;
        w->caseRiseSpeed = 10;
        obj->slot = DrawSlot_Alloc();
        if (obj->slot != -1) {
            obj->node = DrawSlot_Get(obj->slot);
            obj->node->owner = obj;
            obj->node->draw = GunFx_Draw;
        } else {
            StageObj_Free(obj);
        }
        break;
    case 3:
        w->shellsDone = 0;
        for (i = 0; i < 3; i++) {
            w->shellSideSpeed[i] = 2;
            w->shellRiseSpeed[i] = 0x14;
            w->shellPhase[i] = 0;
            if (i == 0) {
                w->shellDelay[i] = 0;
            } else {
                w->shellDelay[i] = 10.0f * Rand_Unit();
            }
            w->shellBounces[i] = 0;
            w->angle[i] = 0;
            w->slot[i] = DrawSlot_Alloc();
            if (w->slot[i] != -1) {
                w->node[i] = DrawSlot_Get(w->slot[i]);
                w->node[i]->owner = obj;
                w->node[i]->part = i;
                w->node[i]->draw = GunFx_Draw;
                offset[0] = 35.714287f;
                offset[1] = 5.714286f;
                offset[2] = 68.57143f;
                Vec_RotateSelf(offset, w->m);
                Mtx_GetTrans(w->node[i]->pos, w->m);
                w->node[i]->pos[0] += offset[0];
                w->node[i]->pos[1] += offset[1];
                w->node[i]->pos[2] += offset[2];
            } else {
                w->node[i] = NULL;
            }
        }
        break;
    case 4:
        w->count = 8;
        for (i = 0; i < w->count; i++, piece++) {
            piece->t = ((s8 *) GunFx_ChargeDelays)[i] - Rand_Next(1) % 2;
            piece->size = ((f32 *) GunFx_ChargeTipSizes)[0];
            piece->slot = DrawSlot_Alloc();
            if (piece->slot != -1) {
                piece->node = DrawSlot_Get(piece->slot);
                piece->node->owner = obj;
                piece->node->part = i;
                piece->node->draw = GunFx_Draw;
                switch (i) {
                case 0:
                case 1:
                    offset[0] = 128.0f;
                    offset[1] = 25.0f;
                    offset[2] = 201.0f;
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                    offset[0] = 105.0f + 10.0f * Rand_Signed();
                    offset[1] = 15.0f + 10.0f * Rand_Signed();
                    offset[2] = 167.0f - 80.0f * Rand_Unit();
                    piece->angle = Rand_Next(1);
                    piece->spin = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                    break;
                case 6:
                case 7:
                    offset[0] = 105.0f;
                    offset[1] = 15.0f;
                    offset[2] = 167.0f;
                    piece->sc[2] = 80.0f + 20.0f * Rand_Unit();
                    piece->angle = Rand_Next(1);
                    break;
                }
                Vec_RotateSelf(offset, w->m);
                Mtx_GetTrans(obj->pos, w->m);
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
            } else {
                piece->node = NULL;
            }
        }
        break;
    case 5:
        w->sprayLevel = 0;
        w->count = 4;
        for (i = 0; i < w->count; i++, piece++) {
            switch (i) {
            case 0:
            case 1:
                piece->size = 1.0f + Rand_Unit();
                piece->angle = Rand_Next(1);
                piece->sc[2] = ((f32 *) GunFx_SprayStreamLengths)[w->sprayLevel] * Rand_Unit() + 2.0f * ((f32 *) GunFx_SprayStreamLengths)[w->sprayLevel];
                offset[0] = ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                offset[1] = -25.0f + ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                offset[2] = 80.0f + ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                piece->t = ((s8 *) GunFx_SprayStreamStarts)[i];
                break;
            case 2:
                piece->size = 0.01f * (100.0f + 20.0f * Rand_Unit());
                piece->angle = Rand_Next(1);
                piece->sc[2] = 100.0f + 20.0f * Rand_Unit();
                offset[0] = ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][0] * Rand_Signed();
                offset[1] = -25.0f + ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][1] * Rand_Signed();
                offset[2] = 80.0f + ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][2] * Rand_Signed();
                piece->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                break;
            case 3:
                piece->size = 1.0f;
                piece->angle = Rand_Next(1);
                piece->sc[2] = ((f32 *) GunFx_SprayDropLengths)[w->sprayLevel] * Rand_Unit() + 4.0f * ((f32 *) GunFx_SprayDropLengths)[w->sprayLevel];
                piece->spin = 0;
                offset[0] = 0.0f;
                offset[1] = -25.0f;
                offset[2] = 80.0f;
                piece->t = -1.0f * (5.0f + 5.0f * Rand_Signed());
                break;
            }
            piece->frame = 0;
            piece->slot = DrawSlot_Alloc();
            if (piece->slot != -1) {
                piece->node = DrawSlot_Get(piece->slot);
                piece->node->owner = obj;
                piece->node->part = i;
                piece->node->draw = GunFx_Draw;
                Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0x1F)));
                Vec_RotateSelf(offset, w->m);
                Mtx_GetTrans(obj->pos, w->m);
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
            } else {
                piece->node = NULL;
            }
        }
        break;
    case 6:
        w->count = 4;
        for (i = 0; i < w->count; i++, piece++) {
            piece->t = ((s8 *) GunFx_BarrelSmokeDelays)[i] - Rand_Next(1) % 2;
            piece->size = ((f32 *) GunFx_BarrelSmokeSizes)[0];
            piece->slot = DrawSlot_Alloc();
            if (piece->slot != -1) {
                piece->node = DrawSlot_Get(piece->slot);
                piece->node->owner = obj;
                piece->node->part = i;
                piece->node->draw = GunFx_Draw;
                piece->angle = Rand_Next(1);
                piece->spin = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                offset[0] = 105.0f;
                offset[1] = 15.0f;
                offset[2] = 167.0f;
                Vec_Copy(pullBack, offset);
                Vec_Normalize(pullBack);
                Vec_Scale(pullBack, pullBack, -10.0f - 70.0f * Rand_Unit());
                offset[0] += pullBack[0];
                offset[1] += pullBack[1];
                offset[2] += pullBack[2];
                offset[0] += 10.0f * Rand_Signed();
                offset[1] += 10.0f * Rand_Signed();
                offset[2] += 10.0f * Rand_Signed();
                Vec_RotateSelf(offset, w->m);
                Mtx_GetTrans(obj->pos, w->m);
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
            } else {
                piece->node = NULL;
            }
        }
        break;
    }
}

/*
 * Returns 1 while a Hermitaur sprays (GunFx type 5): in one of the listed
 * motions when em->unk471 is set or it is enraged, or calm in motion
 * EM_MOTION_ID(0, 0x27) before frame 70; else 0.
 */
s32 func_0088C890(Em *em)
{
    u8 inMotion = 0;

    switch (em->motionId[0]) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F0:
    case 0x3F7:
    case 0x3F8:
    case 0x3FB:
    case 0x3FD:
    case 0x3FF:
    case 0x401:
    case 0x40E:
    case 0x415:
    case 0x41B:
    case 0x41C:
    case 0x41D:
    case 0x41E:
    case 0x420:
        inMotion = 1;
        break;
    }
    if ((EmHermitaur_SprayBoosted(em) == 1 && inMotion == 1)
        || (em->rage == 0 && em->motionId[0] == 0x40F && !EmMotion_ReachedFrame(em, 0, 70.0f))
        || (em->rage == 1 && inMotion == 1)) {
        return 1;
    }
    return 0;
}

void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *a, s32 t, u32 *c);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 func_0088C890(Em *em);
extern u32 GunFx_SmokePuffScaleKeys[], GunFx_SmokePuffColorKeys[], GunFx_SmokeBlastScaleKeys[], GunFx_SmokeBlastColorKeys[], GunFx_SmokeCloudScaleKeys[], GunFx_SmokeCloudColorKeys[];
extern u32 gGunFx_ChargePartFrames[], GunFx_ChargeTipScaleKeys[], GunFx_ChargeTipColorKeys[], GunFx_ChargeGlowScaleKeys[], GunFx_ChargeGlowColorKeys[], GunFx_ChargeFlameScaleKeys[];
extern u32 GunFx_ChargeFlameColorKeys[], GunFx_ChargeJetScaleKeys[], GunFx_ChargeJetColorKeys[], GunFx_SprayPartFrames[];
extern u32 GunFx_SprayStreamScaleKeys[], GunFx_SprayStreamColorKeys[], GunFx_SprayCloudScaleKeys[], GunFx_SprayCloudColorKeys[], GunFx_SprayDropScaleKeys[], GunFx_SprayDropColorKeys[];
extern u32 GunFx_BarrelSmokeScaleKeys[], GunFx_BarrelSmokeColorKeys[];

/*
 * Runs a GunFx object each frame and puts its live parts in a draw table; it
 * ends (state 2) when its time is up. The flash stays 6 frames; the smoke
 * lasts 0x26 to 0x32 frames by shell level, its puffs rising and its blasts
 * moving out with a flipbook and a sound each. The spent case and shells fly
 * out by the owner's heading and fall to his floor (the shells bounce twice).
 * The wyvern charge lasts up to 50 frames while the owner stays in
 * PL_GROUP_WEAPON and ends with a sound; the spray runs while the monster is
 * active, re-placed at its bone; the barrel smoke lasts 0x555 frames while
 * the weapon is out.
 */
void GunFx_Run(StageObj *obj)
{
    GunFxPart *piece;
    GunFxWork *w;
    u8 idle;
    Player *pl;
    u8 i;
    u8 j;
    s8 lifetime;
    void *scaleKeys;
    void *colorKeys;
    f32 keptDepth;
    f32 offset[3];
    f32 step[3];
    f32 pullBack[3];

    w = (GunFxWork *) obj->buf;
    idle = 0;
    piece = w->part;
    switch (obj->type) {
    case 0:
        if (obj->timer++ >= 6) {
            obj->state++;
            obj->timer = 0;
            return;
        }
        offset[0] = 105.0f;
        offset[1] = 15.0f;
        offset[2] = 167.0f;
        Vec_RotateSelf(offset, w->m);
        Mtx_GetTrans(obj->pos, w->m);
        for (i = 0; i < w->count; i++, piece++) {
            if (piece->node != NULL) {
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
                OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
            }
        }
        break;
    case 1:
        switch (obj->var2) {
        case 0:
            lifetime = 0x26;
            break;
        case 1:
            lifetime = 0x2E;
            break;
        case 2:
            lifetime = 0x30;
            break;
        case 3:
            lifetime = 0x32;
            break;
        }
        if (obj->timer++ >= lifetime) {
            obj->state++;
            obj->timer = 0;
            return;
        }
        for (i = 0; i < w->count; i++, piece++) {
            if (piece->node != NULL) {
                if (++piece->t >= 0) {
                    switch (i) {
                    case 0:
                    case 1:
                    case 2:
                        scaleKeys = GunFx_SmokePuffScaleKeys;
                        colorKeys = GunFx_SmokePuffColorKeys;
                        piece->angle += piece->spin;
                        piece->node->pos[1] += 80.0f / lifetime;
                        break;
                    case 3:
                    case 4:
                    case 5:
                    case 9:
                        scaleKeys = GunFx_SmokeBlastScaleKeys;
                        colorKeys = GunFx_SmokeBlastColorKeys;
                        if (piece->frame < 15) {
                            piece->frame++;
                        }
                        step[0] = 0.0f;
                        step[1] = 0.0f;
                        step[2] = 10.0f / lifetime;
                        Vec_RotateY(step, 0.5462881f);
                        Vec_RotateSelf(step, w->m);
                        piece->node->pos[0] += step[0];
                        piece->node->pos[1] += step[1];
                        piece->node->pos[2] += step[2];
                        break;
                    case 6:
                    case 7:
                    case 8:
                    case 10:
                        scaleKeys = GunFx_SmokeCloudScaleKeys;
                        colorKeys = GunFx_SmokeCloudColorKeys;
                        break;
                    }
                    keptDepth = piece->sc[2];
                    Key_LerpVec3(scaleKeys, piece->sc, piece->t);
                    piece->sc[0] *= piece->size;
                    piece->sc[1] *= piece->size;
                    piece->sc[2] = keptDepth;
                    Color_EvalKeys((s32 *)colorKeys, piece->t, (u32 *)&piece->color);
                    OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
                    switch (obj->var2) {
                    case 1:
                        if (i == 3 && piece->sc[2] < 2.0f) {
                            piece->sc[2] = 1.0f;
                        }
                        break;
                    case 2:
                        if ((i == 3 || i == 4 || i == 5) && piece->sc[2] < 2.0f) {
                            piece->sc[2] = 1.0f;
                        }
                        break;
                    case 3:
                        if ((i == 3 || i == 4 || i == 5 || i == 9) && piece->sc[2] < 2.0f) {
                            piece->sc[2] = 1.0f;
                        }
                        break;
                    }
                }
                if (piece->sc[2] == 1.0f) {
                    switch (obj->var2) {
                    case 1:
                        Sound_PlayPlayerSe(obj->owner, 0x85, 0, piece->node->pos, 1, 0);
                        piece->sc[2] += 1.0f;
                        break;
                    case 2:
                        Sound_PlayPlayerSe(obj->owner, 0x86, 0, piece->node->pos, 1, 0);
                        piece->sc[2] += 1.0f;
                        break;
                    case 3:
                        Sound_PlayPlayerSe(obj->owner, 0x87, 0, piece->node->pos, 1, 0);
                        piece->sc[2] += 1.0f;
                        break;
                    }
                }
            }
        }
        break;
    case 2:
        w->caseRiseSpeed += -2.5f;
        step[0] = w->caseSideSpeed;
        step[1] = w->caseRiseSpeed;
        step[2] = 0.0f;
        Vec_RotateY(step, (6.2831855f * ((Player *) obj->owner)->rot[1]) / 65536.0f);
        obj->pos[0] += step[0];
        obj->pos[1] += step[1];
        obj->pos[2] += step[2];
        Vec_Copy(obj->node->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->node, 0x40, 0);
        if (obj->pos[1] <= ((Player *) obj->owner)->floorY) {
            obj->state++;
        }
        break;
    case 3:
        w->shellsDone = 0;
        for (i = 0; i < 3; i++) {
            if (w->node[i] == NULL) {
                w->shellsDone++;
            } else {
                switch (w->shellPhase[i]) {
                case 0:
                    if (--w->shellDelay[i] <= 0) {
                        w->shellPhase[i]++;
                    }
                    break;
                case 1:
                    w->shellRiseSpeed[i] += -2.5f;
                    step[0] = w->shellSideSpeed[i];
                    step[1] = w->shellRiseSpeed[i];
                    step[2] = 0.0f;
                    Vec_RotateY(step, (6.2831855f * ((Player *) obj->owner)->rot[1]) / 65536.0f + (6.2831855f * (120.0f * i)) / 360.0f);
                    w->node[i]->pos[0] += step[0];
                    w->node[i]->pos[1] += step[1];
                    w->node[i]->pos[2] += step[2];
                    w->angle[i] += 0x2000;
                    if (w->node[i]->pos[1] <= ((Player *) obj->owner)->floorY && w->shellRiseSpeed[i] <= 0) {
                        w->shellBounces[i]++;
                        w->shellPhase[i]++;
                        w->node[i]->pos[1] = ((Player *) obj->owner)->floorY;
                    }
                    OT_AddByDepth(gDrawOt32, w->node[i], 0x40, 0);
                    break;
                case 2:
                    if (w->shellBounces[i] < 2) {
                        w->shellSideSpeed[i] = 1.25f + 0.1f * (10.0f * Rand_Unit());
                        w->shellRiseSpeed[i] = 10.0f + 5.0f * Rand_Unit();
                        w->shellPhase[i] = 1;
                    } else {
                        w->shellPhase[i]++;
                    }
                    break;
                case 3:
                    w->shellsDone++;
                    break;
                }
            }
        }
        if (w->shellsDone == 3) {
            obj->state++;
        }
        break;
    case 4:
        if (obj->timer++ >= 50 || ((Player *) obj->owner)->stateGroup != 1) {
            Player_PlaySe(obj->owner, 0x1E, 0, obj->pos, 1, 0);
            obj->state++;
            obj->timer = 0;
            return;
        }
        for (i = 0; i < w->count; i++, piece++) {
            if (piece->node != NULL) {
                if (++piece->t >= 0) {
                    switch (i) {
                    case 0:
                        piece->size = ((f32 *) GunFx_ChargeTipSizes)[piece->t % 2];
                        scaleKeys = GunFx_ChargeTipScaleKeys;
                        colorKeys = GunFx_ChargeTipColorKeys;
                        break;
                    case 1:
                        piece->size = ((f32 *) GunFx_ChargeTipSizes)[piece->t % 2];
                        scaleKeys = GunFx_ChargeGlowScaleKeys;
                        colorKeys = GunFx_ChargeGlowColorKeys;
                        break;
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        if (piece->t > ((s8 *) gGunFx_ChargePartFrames)[i] && obj->timer < 35) {
                            piece->t = 0;
                            offset[0] = 105.0f + 10.0f * Rand_Signed();
                            offset[1] = 15.0f + 10.0f * Rand_Signed();
                            offset[2] = 167.0f - 80.0f * Rand_Unit();
                            piece->angle = Rand_Next(1);
                            piece->spin = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                            Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0xE)));
                            Vec_RotateSelf(offset, w->m);
                            Mtx_GetTrans(obj->pos, w->m);
                            piece->node->pos[0] = obj->pos[0] + offset[0];
                            piece->node->pos[1] = obj->pos[1] + offset[1];
                            piece->node->pos[2] = obj->pos[2] + offset[2];
                        }
                        piece->size = 1.0f;
                        scaleKeys = GunFx_ChargeFlameScaleKeys;
                        colorKeys = GunFx_ChargeFlameColorKeys;
                        piece->angle += piece->spin;
                        piece->node->pos[1] += 30.0f / ((s8 *) gGunFx_ChargePartFrames)[i];
                        break;
                    case 6:
                    case 7:
                        if (piece->t > ((s8 *) gGunFx_ChargePartFrames)[i] && obj->timer < 26) {
                            piece->t = 2.0f * Rand_Unit() - 4.0f;
                            piece->sc[2] = 80.0f + 20.0f * Rand_Unit();
                            piece->angle = Rand_Next(1);
                            offset[0] = 105.0f;
                            offset[1] = 15.0f;
                            offset[2] = 167.0f;
                            Vec_RotateSelf(offset, w->m);
                            Mtx_GetTrans(obj->pos, w->m);
                            piece->node->pos[0] = obj->pos[0] + offset[0];
                            piece->node->pos[1] = obj->pos[1] + offset[1];
                            piece->node->pos[2] = obj->pos[2] + offset[2];
                        }
                        piece->size = 1.0f;
                        keptDepth = piece->sc[2];
                        scaleKeys = GunFx_ChargeJetScaleKeys;
                        colorKeys = GunFx_ChargeJetColorKeys;
                        step[0] = 0.0f;
                        step[1] = 0.0f;
                        step[2] = keptDepth / ((s8 *) gGunFx_ChargePartFrames)[i];
                        Vec_RotateY(step, 0.5462881f);
                        Vec_RotateSelf(step, w->m);
                        piece->node->pos[0] += step[0];
                        piece->node->pos[1] += step[1];
                        piece->node->pos[2] += step[2];
                        break;
                    }
                    Key_LerpVec3(scaleKeys, piece->sc, piece->t);
                    piece->sc[0] *= piece->size;
                    piece->sc[1] *= piece->size;
                    piece->sc[2] *= piece->size;
                    Color_EvalKeys((s32 *)colorKeys, piece->t, (u32 *)&piece->color);
                    if (i < 2) {
                        offset[0] = 128.0f;
                        offset[1] = 25.0f;
                        offset[2] = 201.0f;
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0xE)));
                        Vec_RotateSelf(offset, w->m);
                        Mtx_GetTrans(piece->node->pos, w->m);
                        piece->node->pos[0] += offset[0];
                        piece->node->pos[1] += offset[1];
                        piece->node->pos[2] += offset[2];
                    }
                    if (i == 6 || i == 7) {
                        piece->sc[2] = keptDepth;
                    }
                    OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
                }
            }
        }
        break;
    case 5:
        pl = obj->owner;
        if (pl->active == 0) {
            obj->state++;
            obj->timer = 0;
            return;
        }
        if (!Player_InCurrentArea(pl)) {
            return;
        }
        j = 0;
        if (func_0088C890((Em *) pl) == 1) {
            if (EmHermitaur_SprayBoosted((Em *) pl) == 1) {
                w->sprayLevel = 1;
            } else {
                w->sprayLevel = 0;
            }
            piece = w->part;
            for (i = 0; i < w->count; i++, piece++) {
                if (piece->node != NULL) {
                    piece->frame = 0;
                }
            }
        } else {
            idle = 1;
            for (i = 0; i < w->count; i++, piece++) {
                if (piece->node == NULL) {
                    if (j++ >= w->count) {
                        return;
                    }
                    continue;
                }
                if (piece->t >= ((s8 *) GunFx_SprayPartFrames)[i] || piece->t < 0) {
                    piece->frame = 1;
                }
                if (piece->frame == 1) {
                    switch (i) {
                    case 0:
                    case 1:
                        piece->t = ((s8 *) GunFx_SprayStreamStarts)[i];
                        break;
                    case 2:
                        piece->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                        break;
                    case 3:
                        piece->t = -1.0f * (5.0f + 5.0f * Rand_Signed());
                        break;
                    }
                    offset[0] = 0.0f;
                    offset[1] = -25.0f;
                    offset[2] = 80.0f;
                    Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0x1F)));
                    Vec_RotateSelf(offset, w->m);
                    Mtx_GetTrans(obj->pos, w->m);
                    piece->node->pos[0] = obj->pos[0] + offset[0];
                    piece->node->pos[1] = obj->pos[1] + offset[1];
                    piece->node->pos[2] = obj->pos[2] + offset[2];
                    if (j++ >= w->count) {
                        return;
                    }
                }
            }
        }
        if (++obj->timer % 30 == 0) {
            if (w->sprayLevel == 1 && idle == 0) {
                Em_PlaySeChecked((struct Em *)pl, 0x2F, 0, obj->pos, 1, 0);
            }
            obj->timer = 0;
        }
        piece = w->part;
        for (i = 0; i < w->count; i++, piece++) {
            if (piece->frame != 1 && piece->node != NULL) {
                if (++piece->t >= 0) {
                    switch (i) {
                    case 0:
                    case 1:
                        if (piece->t > ((s8 *) GunFx_SprayPartFrames)[i]) {
                            piece->size = 1.0f + Rand_Unit();
                            piece->angle = Rand_Next(1);
                            offset[0] = ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                            offset[1] = -25.0f + ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                            offset[2] = 80.0f + ((f32 *) GunFx_SprayStreamSpreads)[w->sprayLevel] * Rand_Signed();
                            piece->sc[2] = ((f32 *) GunFx_SprayStreamLengths)[w->sprayLevel] * Rand_Unit() + 2.0f * ((f32 *) GunFx_SprayStreamLengths)[w->sprayLevel];
                        }
                        keptDepth = piece->sc[2];
                        scaleKeys = ((void **) GunFx_SprayStreamScaleKeys)[w->sprayLevel];
                        colorKeys = ((void **) GunFx_SprayStreamColorKeys)[w->sprayLevel];
                        step[0] = 0.0f;
                        step[1] = 0.0f;
                        step[2] = keptDepth / ((s8 *) GunFx_SprayPartFrames)[i];
                        break;
                    case 2:
                        if (piece->t > ((s8 *) GunFx_SprayPartFrames)[i]) {
                            piece->size = 0.01f * (100.0f + 20.0f * Rand_Unit());
                            offset[0] = ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][0] * Rand_Signed();
                            offset[1] = -25.0f + ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][1] * Rand_Signed();
                            offset[2] = 80.0f + ((f32 (*)[3]) GunFx_SprayCloudSpreads)[w->sprayLevel][2] * Rand_Signed();
                            piece->sc[2] = 100.0f + 20.0f * Rand_Unit();
                        }
                        keptDepth = piece->sc[2];
                        scaleKeys = ((void **) GunFx_SprayCloudScaleKeys)[w->sprayLevel];
                        colorKeys = ((void **) GunFx_SprayCloudColorKeys)[w->sprayLevel];
                        step[0] = 0.0f;
                        step[1] = (-1.0f * keptDepth) / ((s8 *) GunFx_SprayPartFrames)[i];
                        step[2] = 0.0f;
                        break;
                    case 3:
                        if (piece->t > ((s8 *) GunFx_SprayPartFrames)[i]) {
                            offset[0] = 0.0f;
                            offset[1] = -25.0f;
                            offset[2] = 80.0f;
                            piece->sc[2] = ((f32 *) GunFx_SprayDropLengths)[w->sprayLevel] * Rand_Unit() + 4.0f * ((f32 *) GunFx_SprayDropLengths)[w->sprayLevel];
                            piece->spin = 0;
                        }
                        keptDepth = piece->sc[2];
                        scaleKeys = ((void **) GunFx_SprayDropScaleKeys)[w->sprayLevel];
                        colorKeys = ((void **) GunFx_SprayDropColorKeys)[w->sprayLevel];
                        piece->spin -= 5;
                        step[0] = 0.0f;
                        step[1] = piece->spin;
                        step[2] = keptDepth / ((s8 *) GunFx_SprayPartFrames)[i];
                        break;
                    }
                    if (piece->t > ((s8 *) GunFx_SprayPartFrames)[i]) {
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0x1F)));
                        Vec_RotateSelf(offset, w->m);
                        Mtx_GetTrans(obj->pos, w->m);
                        piece->node->pos[0] = obj->pos[0] + offset[0];
                        piece->node->pos[1] = obj->pos[1] + offset[1];
                        piece->node->pos[2] = obj->pos[2] + offset[2];
                        switch (i) {
                        case 0:
                        case 1:
                            piece->t = 0;
                            break;
                        case 2:
                            piece->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                            break;
                        case 3:
                            piece->t = -1.0f * (5.0f + 5.0f * Rand_Unit());
                            break;
                        }
                    }
                    Vec_RotateSelf(step, w->m);
                    piece->node->pos[0] += step[0];
                    piece->node->pos[1] += step[1];
                    piece->node->pos[2] += step[2];
                    Key_LerpVec3(scaleKeys, piece->sc, piece->t);
                    piece->sc[0] *= piece->size;
                    piece->sc[1] *= piece->size;
                    piece->sc[2] *= piece->size;
                    Color_EvalKeys((s32 *)colorKeys, piece->t, (u32 *)&piece->color);
                    piece->sc[2] = keptDepth;
                    OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
                }
            }
        }
        break;
    case 6:
        if (obj->timer++ >= 0x555) {
            obj->state++;
            obj->timer = 0;
            return;
        }
        if (((Player *) obj->owner)->weaponOut != 0) {
            for (i = 0; i < w->count; i++, piece++) {
                if (piece->node != NULL) {
                    if (++piece->t > 0) {
                        if (obj->timer < 0x546 && piece->t > 15) {
                            piece->t = -5;
                        }
                        piece->size = 1.0f;
                        piece->angle += piece->spin;
                        piece->node->pos[1] += 2.0f;
                        Key_LerpVec3((f32 *)GunFx_BarrelSmokeScaleKeys, piece->sc, piece->t);
                        piece->sc[0] *= piece->size;
                        piece->sc[1] *= piece->size;
                        piece->sc[2] *= piece->size;
                        Color_EvalKeys((s32 *)GunFx_BarrelSmokeColorKeys, piece->t, (u32 *)&piece->color);
                        OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
                    } else if (piece->t <= 0) {
                        offset[0] = 105.0f;
                        offset[1] = 15.0f;
                        offset[2] = 167.0f;
                        Vec_Copy(pullBack, offset);
                        Vec_Normalize(pullBack);
                        Vec_Scale(pullBack, pullBack, -10.0f - 70.0f * Rand_Unit());
                        offset[0] += pullBack[0];
                        offset[1] += pullBack[1];
                        offset[2] += pullBack[2];
                        offset[0] += 10.0f * Rand_Signed();
                        offset[1] += 10.0f * Rand_Signed();
                        offset[2] += 10.0f * Rand_Signed();
                        piece->angle = Rand_Next(1);
                        piece->spin = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(obj->owner, 0xE)));
                        Vec_RotateSelf(offset, w->m);
                        Mtx_GetTrans(obj->pos, w->m);
                        piece->node->pos[0] = obj->pos[0] + offset[0];
                        piece->node->pos[1] = obj->pos[1] + offset[1];
                        piece->node->pos[2] = obj->pos[2] + offset[2];
                    }
                }
            }
        }
        break;
    }
}

/* The file's data. */
f32 MashDebris_SpinDeg[6] = {
#include "game/008D53B0.inc"
};
u8 MashDebris_PartFalls[8] = {
#include "game/008D53C8.inc"
};
u32 MashDebris_AlphaKeys_0[8] = {
#include "game/008D53D0.inc"
};
u32 MashDebris_AlphaKeys_1[8] = {
#include "game/008D53F0.inc"
};
u32 MashDebris_AlphaKeys_2[12] = {
#include "game/008D5410.inc"
};
u32 MashDebris_AlphaKeys_3[8] = {
#include "game/008D5440.inc"
};
u32 MashDebris_AlphaKeys_4[8] = {
#include "game/008D5460.inc"
};
void *MashDebris_AlphaKeys[8] = {
    (void *)MashDebris_AlphaKeys_0, (void *)MashDebris_AlphaKeys_1, (void *)MashDebris_AlphaKeys_2, (void *)MashDebris_AlphaKeys_3,
    (void *)MashDebris_AlphaKeys_4, NULL, NULL, NULL,
};
u32 MashDebris_ScaleKeys_1[16] = {
#include "game/008D54A0.inc"
};
u32 MashDebris_ScaleKeys_2[16] = {
#include "game/008D54E0.inc"
};
u32 MashDebris_ScaleKeys_3[16] = {
#include "game/008D5520.inc"
};
void *MashDebris_ScaleKeys[8] = {
    NULL, (void *)MashDebris_ScaleKeys_1, (void *)MashDebris_ScaleKeys_2, (void *)MashDebris_ScaleKeys_3,
    NULL, NULL, NULL, NULL,
};
u8 MashDebris_TrickleScaleKeys[48] = {
#include "game/008D5580.inc"
};
f32 gStageObjK1C_ScaleCurves_0[12] = {
#include "game/008D55B0.inc"
};
u8 gStageObjK1C_ColorKeys_0[48] = {
#include "game/008D55E0.inc"
};
f32 gStageObjK1C_ScaleCurves_1[8] = {
#include "game/008D5610.inc"
};
f32 gStageObjK1C_ScaleCurves_3[12] = {
#include "game/008D5630.inc"
};
f32 gStageObjK1C_ScaleCurves_5[16] = {
#include "game/008D5660.inc"
};
f32 gStageObjK1C_ScaleCurves_7[12] = {
#include "game/008D56A0.inc"
};
f32 gStageObjK1C_ScaleCurves_9[16] = {
#include "game/008D56D0.inc"
};
f32 gStageObjK1C_ScaleCurves_2[8] = {
#include "game/008D5710.inc"
};
f32 gStageObjK1C_ScaleCurves_4[8] = {
#include "game/008D5730.inc"
};
u8 gStageObjK1C_ColorKeys_1[32] = {
#include "game/008D5750.inc"
};
u8 gStageObjK1C_ColorKeys_3[32] = {
#include "game/008D5770.inc"
};
u8 gStageObjK1C_ColorKeys_5[40] = {
#include "game/008D5790.inc"
};
u32 gStageObjK1C_SeqTbls_1[3] = {
#include "game/008D57B8.inc"
};
u32 MonsterFx_BlastDelays[8] = {
#include "game/008D57D0.inc"
};
u32 MonsterFx_BlastRises[12] = {
#include "game/008D57F0.inc"
};
u32 MonsterFx_BlastSizes[10] = {
#include "game/008D5820.inc"
};
u32 gStageObjK1C_DrawTbls_1[3] = {
#include "game/008D5848.inc"
};
f32 gStageObjK1C_ScaleCurves_12[16] = {
#include "game/008D5860.inc"
};
f32 gStageObjK1C_ScaleCurves_15[12] = {
#include "game/008D58A0.inc"
};
f32 gStageObjK1C_ScaleCurves_16[12] = {
#include "game/008D58D0.inc"
};
u8 gStageObjK1C_ColorKeys_8[48] = {
#include "game/008D5900.inc"
};
u8 gStageObjK1C_ColorKeys_11[48] = {
#include "game/008D5930.inc"
};
u8 gStageObjK1C_ColorKeys_12[40] = {
#include "game/008D5960.inc"
};
u32 gStageObjK1C_SeqTbls_2[2] = {
#include "game/008D5988.inc"
};
u32 MonsterFx_ShardsDelays[4] = {
#include "game/008D5990.inc"
};
u32 gStageObjK1C_DrawTbls_2[4] = {
#include "game/008D59A0.inc"
};
f32 gStageObjK1C_ScaleCurves_17[16] = {
#include "game/008D59B0.inc"
};
f32 gStageObjK1C_ScaleCurves_18[12] = {
#include "game/008D59F0.inc"
};
u8 gStageObjK1C_ColorKeys_13[32] = {
#include "game/008D5A20.inc"
};
u8 gStageObjK1C_ColorKeys_14[24] = {
#include "game/008D5A40.inc"
};
u32 gStageObjK1C_DrawTbls_3[2] = {
#include "game/008D5A58.inc"
};
f32 gStageObjK1C_ScaleCurves_19[12] = {
#include "game/008D5A60.inc"
};
u8 gStageObjK1C_ColorKeys_15[40] = {
#include "game/008D5A90.inc"
};
u32 gStageObjK1C_DrawTbls_4[2] = {
#include "game/008D5AB8.inc"
};
f32 gStageObjK1C_ScaleCurves_20[20] = {
#include "game/008D5AC0.inc"
};
f32 gStageObjK1C_ScaleCurves_21[12] = {
#include "game/008D5B10.inc"
};
u8 gStageObjK1C_ColorKeys_16[32] = {
#include "game/008D5B40.inc"
};
u8 gStageObjK1C_ColorKeys_17[40] = {
#include "game/008D5B60.inc"
};
u32 MonsterFx_GroundSprayKinds[2] = {
#include "game/008D5B88.inc"
};
u32 MonsterFx_GroundSprayRises[4] = {
#include "game/008D5B90.inc"
};
u32 gStageObjK1C_SeqTbls_5[2] = {
#include "game/008D5BA0.inc"
};
u32 gStageObjK1C_DrawTbls_5[2] = {
#include "game/008D5BA8.inc"
};
u32 gStageObjK1C_DrawTbls_6[4] = {
#include "game/008D5BB0.inc"
};
f32 gStageObjK1C_ScaleCurves_24[12] = {
#include "game/008D5BC0.inc"
};
f32 gStageObjK1C_ScaleCurves_25[8] = {
#include "game/008D5BF0.inc"
};
u8 gStageObjK1C_ColorKeys_20[32] = {
#include "game/008D5C10.inc"
};
u8 gStageObjK1C_ColorKeys_21[32] = {
#include "game/008D5C30.inc"
};
f32 gStageObjK1C_ScaleCurves_27[12] = {
#include "game/008D5C50.inc"
};
f32 gStageObjK1C_ScaleCurves_28[8] = {
#include "game/008D5C80.inc"
};
u8 gStageObjK1C_ColorKeys_23[32] = {
#include "game/008D5CA0.inc"
};
u8 gStageObjK1C_ColorKeys_24[24] = {
#include "game/008D5CC0.inc"
};
u32 gStageObjK1C_DrawTbls_7[2] = {
#include "game/008D5CD8.inc"
};
u32 gStageObjK1C_SeqTbls_7[4] = {
#include "game/008D5CE0.inc"
};
f32 gStageObjK1C_ScaleCurves_30[16] = {
#include "game/008D5CF0.inc"
};
f32 gStageObjK1C_ScaleCurves_31[8] = {
#include "game/008D5D30.inc"
};
f32 gStageObjK1C_ScaleCurves_32[8] = {
#include "game/008D5D50.inc"
};
u8 gStageObjK1C_ColorKeys_26[32] = {
#include "game/008D5D70.inc"
};
u8 gStageObjK1C_ColorKeys_27[32] = {
#include "game/008D5D90.inc"
};
u8 gStageObjK1C_ColorKeys_28[40] = {
#include "game/008D5DB0.inc"
};
u32 gStageObjK1C_DrawTbls_8[2] = {
#include "game/008D5DD8.inc"
};
u32 gStageObjK1C_SeqTbls_8[2] = {
#include "game/008D5DE0.inc"
};
u32 MonsterFx_StrikeDepths[2] = {
#include "game/008D5DE8.inc"
};
f32 gStageObjK1C_ScaleCurves_34[8] = {
#include "game/008D5DF0.inc"
};
f32 gStageObjK1C_ScaleCurves_35[12] = {
#include "game/008D5E10.inc"
};
f32 gStageObjK1C_ScaleCurves_38[8] = {
#include "game/008D5E40.inc"
};
u8 gStageObjK1C_ColorKeys_30[48] = {
#include "game/008D5E60.inc"
};
u8 gStageObjK1C_ColorKeys_31[48] = {
#include "game/008D5E90.inc"
};
u8 gStageObjK1C_ColorKeys_34[24] = {
#include "game/008D5EC0.inc"
};
u32 gStageObjK1C_DrawTbls_9[2] = {
#include "game/008D5ED8.inc"
};
u32 gStageObjK1C_SeqTbls_9[4] = {
#include "game/008D5EE0.inc"
};
f32 gStageObjK1C_ScaleCurves_39[16] = {
#include "game/008D5EF0.inc"
};
f32 gStageObjK1C_ScaleCurves_40[8] = {
#include "game/008D5F30.inc"
};
f32 gStageObjK1C_ScaleCurves_41[12] = {
#include "game/008D5F50.inc"
};
u8 gStageObjK1C_ColorKeys_35[32] = {
#include "game/008D5F80.inc"
};
u8 gStageObjK1C_ColorKeys_36[32] = {
#include "game/008D5FA0.inc"
};
u8 gStageObjK1C_ColorKeys_37[32] = {
#include "game/008D5FC0.inc"
};
u32 gStageObjK1C_DrawTbls_10[2] = {
#include "game/008D5FE0.inc"
};
u32 gStageObjK1C_SeqTbls_10[2] = {
#include "game/008D5FE8.inc"
};
u32 MonsterFx_KushalaGustDelays[4] = {
#include "game/008D5FF0.inc"
};
f32 gStageObjK1C_ScaleCurves_43[12] = {
#include "game/008D6000.inc"
};
f32 gStageObjK1C_ScaleCurves_44[16] = {
#include "game/008D6030.inc"
};
f32 gStageObjK1C_ScaleCurves_45[12] = {
#include "game/008D6070.inc"
};
u8 gStageObjK1C_ColorKeys_39[32] = {
#include "game/008D60A0.inc"
};
u8 gStageObjK1C_ColorKeys_42[32] = {
#include "game/008D60C0.inc"
};
u8 gStageObjK1C_ColorKeys_40[32] = {
#include "game/008D60E0.inc"
};
u8 gStageObjK1C_ColorKeys_41[40] = {
#include "game/008D6100.inc"
};
u32 gStageObjK1C_DrawTbls_12[2] = {
#include "game/008D6128.inc"
};
u16 MonsterFx_KushalaRingFrames[16] = {
#include "game/008D6130.inc"
};
u16 MonsterFx_KushalaRingStarts[16] = {
#include "game/008D6150.inc"
};
f32 gStageObjK1C_ScaleCurves_46[12] = {
#include "game/008D6170.inc"
};
f32 gStageObjK1C_ScaleCurves_47[8] = {
#include "game/008D61A0.inc"
};
u8 gStageObjK1C_ColorKeys_43[32] = {
#include "game/008D61C0.inc"
};
u8 gStageObjK1C_ColorKeys_44[32] = {
#include "game/008D61E0.inc"
};
f32 gStageObjK1C_ScaleCurves_49[12] = {
#include "game/008D6200.inc"
};
f32 gStageObjK1C_ScaleCurves_50[8] = {
#include "game/008D6230.inc"
};
u8 gStageObjK1C_ColorKeys_46[32] = {
#include "game/008D6250.inc"
};
u8 gStageObjK1C_ColorKeys_47[32] = {
#include "game/008D6270.inc"
};
f32 gStageObjK1C_ScaleCurves_52[12] = {
#include "game/008D6290.inc"
};
f32 gStageObjK1C_ScaleCurves_53[8] = {
#include "game/008D62C0.inc"
};
u8 gStageObjK1C_ColorKeys_49[32] = {
#include "game/008D62E0.inc"
};
u8 gStageObjK1C_ColorKeys_50[32] = {
#include "game/008D6300.inc"
};
u32 gStageObjK1C_DrawTbls_13[2] = {
#include "game/008D6320.inc"
};
u32 gStageObjK1C_SeqTbls_13[2] = {
#include "game/008D6328.inc"
};
f32 gStageObjK1C_ScaleCurves_55[12] = {
#include "game/008D6330.inc"
};
f32 gStageObjK1C_ScaleCurves_56[8] = {
#include "game/008D6360.inc"
};
u8 gStageObjK1C_ColorKeys_52[32] = {
#include "game/008D6380.inc"
};
u8 gStageObjK1C_ColorKeys_53[32] = {
#include "game/008D63A0.inc"
};
f32 gStageObjK1C_ScaleCurves_58[8] = {
#include "game/008D63C0.inc"
};
f32 gStageObjK1C_ScaleCurves_59[12] = {
#include "game/008D63E0.inc"
};
f32 gStageObjK1C_ScaleCurves_60[12] = {
#include "game/008D6410.inc"
};
u8 gStageObjK1C_ColorKeys_55[32] = {
#include "game/008D6440.inc"
};
u8 gStageObjK1C_ColorKeys_56[32] = {
#include "game/008D6460.inc"
};
u8 gStageObjK1C_ColorKeys_57[24] = {
#include "game/008D6480.inc"
};
u32 gStageObjK1C_DrawTbls_14[2] = {
#include "game/008D6498.inc"
};
u32 gStageObjK1C_SeqTbls_14[4] = {
#include "game/008D64A0.inc"
};
f32 gStageObjK1C_ScaleCurves_62[8] = {
#include "game/008D64B0.inc"
};
f32 gStageObjK1C_ScaleCurves_63[16] = {
#include "game/008D64D0.inc"
};
f32 gStageObjK1C_ScaleCurves_64[12] = {
#include "game/008D6510.inc"
};
f32 gStageObjK1C_ScaleCurves_65[12] = {
#include "game/008D6540.inc"
};
f32 gStageObjK1C_ScaleCurves_66[12] = {
#include "game/008D6570.inc"
};
f32 gStageObjK1C_ScaleCurves_68[12] = {
#include "game/008D65A0.inc"
};
f32 gStageObjK1C_ScaleCurves_70[16] = {
#include "game/008D65D0.inc"
};
u8 gStageObjK1C_ColorKeys_59[32] = {
#include "game/008D6610.inc"
};
u8 gStageObjK1C_ColorKeys_60[32] = {
#include "game/008D6630.inc"
};
u8 gStageObjK1C_ColorKeys_61[32] = {
#include "game/008D6650.inc"
};
u8 gStageObjK1C_ColorKeys_62[32] = {
#include "game/008D6670.inc"
};
u8 gStageObjK1C_ColorKeys_63[32] = {
#include "game/008D6690.inc"
};
u8 gStageObjK1C_ColorKeys_65[48] = {
#include "game/008D66B0.inc"
};
u8 gStageObjK1C_ColorKeys_67[32] = {
#include "game/008D66E0.inc"
};
u32 gStageObjK1C_DrawTbls_15[8] = {
#include "game/008D6700.inc"
};
s16 MonsterFx_DustBurstFrames[24] = {
#include "game/008D6720.inc"
};
u16 MonsterFx_BreathDustStarts[16] = {
#include "game/008D6750.inc"
};
f32 gStageObjK1C_ScaleCurves_71[16] = {
#include "game/008D6770.inc"
};
f32 gStageObjK1C_ScaleCurves_72[12] = {
#include "game/008D67B0.inc"
};
u8 gStageObjK1C_ColorKeys_68[32] = {
#include "game/008D67E0.inc"
};
u8 gStageObjK1C_ColorKeys_69[40] = {
#include "game/008D6800.inc"
};
u32 gStageObjK1C_DrawTbls_16[2] = {
#include "game/008D6828.inc"
};
u32 MonsterFx_KushalaGroundWindFrames[2] = {
#include "game/008D6830.inc"
};
u8 MonsterFx_ImpactDustKinds[12] = {
#include "game/008D6838.inc"
};
u16 MonsterFx_ImpactDustStarts[16] = {
#include "game/008D6850.inc"
};
f32 gStageObjK1C_ScaleCurves_73[16] = {
#include "game/008D6870.inc"
};
f32 gStageObjK1C_ScaleCurves_74[12] = {
#include "game/008D68B0.inc"
};
u8 gStageObjK1C_ColorKeys_70[32] = {
#include "game/008D68E0.inc"
};
u8 gStageObjK1C_ColorKeys_71[40] = {
#include "game/008D6900.inc"
};
u32 gStageObjK1C_DrawTbls_19[2] = {
#include "game/008D6928.inc"
};
u32 gStageObjK1C_SeqTbls_19[2] = {
#include "game/008D6930.inc"
};
u8 MonsterFx_DustKickKinds[12] = {
#include "game/008D6938.inc"
};
u16 MonsterFx_DustKickStarts[16] = {
#include "game/008D6950.inc"
};
f32 gStageObjK1C_ScaleCurves_76[12] = {
#include "game/008D6970.inc"
};
u8 gStageObjK1C_ColorKeys_73[40] = {
#include "game/008D69A0.inc"
};
u32 gStageObjK1C_DrawTbls_20[2] = {
#include "game/008D69C8.inc"
};
u32 gStageObjK1C_SeqTbls_20[4] = {
#include "game/008D69D0.inc"
};
f32 gStageObjK1C_ScaleCurves_77[8] = {
#include "game/008D69E0.inc"
};
u8 gStageObjK1C_ColorKeys_74[32] = {
#include "game/008D6A00.inc"
};
u32 gStageObjK1C_DrawTbls_22[2] = {
#include "game/008D6A20.inc"
};
u32 gStageObjK1C_SeqTbls_22[2] = {
#include "game/008D6A28.inc"
};
f32 gStageObjK1C_ScaleCurves_78[12] = {
#include "game/008D6A30.inc"
};
u8 gStageObjK1C_ColorKeys_75[32] = {
#include "game/008D6A60.inc"
};
u8 gStageObjK1C_ColorKeys_76[32] = {
#include "game/008D6A80.inc"
};
u8 gStageObjK1C_ColorKeys_77[32] = {
#include "game/008D6AA0.inc"
};
u8 gStageObjK1C_ColorKeys_78[24] = {
#include "game/008D6AC0.inc"
};
u32 gStageObjK1C_DrawTbls_23[2] = {
#include "game/008D6AD8.inc"
};
u32 gStageObjK1C_SeqTbls_23[4] = {
#include "game/008D6AE0.inc"
};
f32 gStageObjK1C_ScaleCurves_79[16] = {
#include "game/008D6AF0.inc"
};
f32 gStageObjK1C_ScaleCurves_80[12] = {
#include "game/008D6B30.inc"
};
u8 gStageObjK1C_ColorKeys_79[32] = {
#include "game/008D6B60.inc"
};
u8 gStageObjK1C_ColorKeys_80[32] = {
#include "game/008D6B80.inc"
};
u32 MonsterFx_RajangRageStartKinds[2] = {
#include "game/008D6BA0.inc"
};
u32 gStageObjK1C_DrawTbls_24[2] = {
#include "game/008D6BA8.inc"
};
u32 gStageObjK1C_SeqTbls_24[4] = {
#include "game/008D6BB0.inc"
};
f32 gStageObjK1C_ScaleCurves_81[12] = {
#include "game/008D6BC0.inc"
};
f32 gStageObjK1C_ScaleCurves_82[12] = {
#include "game/008D6BF0.inc"
};
u8 gStageObjK1C_ColorKeys_81[32] = {
#include "game/008D6C20.inc"
};
u8 gStageObjK1C_ColorKeys_82[32] = {
#include "game/008D6C40.inc"
};
u32 gStageObjK1C_DrawTbls_25[2] = {
#include "game/008D6C60.inc"
};
u32 gStageObjK1C_SeqTbls_25[2] = {
#include "game/008D6C68.inc"
};
f32 gStageObjK1C_ScaleCurves_83[8] = {
#include "game/008D6C70.inc"
};
f32 gStageObjK1C_ScaleCurves_84[12] = {
#include "game/008D6C90.inc"
};
f32 gStageObjK1C_ScaleCurves_85[16] = {
#include "game/008D6CC0.inc"
};
u8 gStageObjK1C_ColorKeys_83[32] = {
#include "game/008D6D00.inc"
};
u8 gStageObjK1C_ColorKeys_84[32] = {
#include "game/008D6D20.inc"
};
u8 gStageObjK1C_ColorKeys_85[24] = {
#include "game/008D6D40.inc"
};
u32 gStageObjK1C_DrawTbls_26[2] = {
#include "game/008D6D58.inc"
};
u32 gStageObjK1C_SeqTbls_26[2] = {
#include "game/008D6D60.inc"
};
u32 MonsterFx_ShenGaorenPuffDelays[2] = {
#include "game/008D6D68.inc"
};
f32 gStageObjK1C_ScaleCurves_86[16] = {
#include "game/008D6D70.inc"
};
u8 gStageObjK1C_ColorKeys_86[24] = {
#include "game/008D6DB0.inc"
};
u32 gStageObjK1C_DrawTbls_27[2] = {
#include "game/008D6DC8.inc"
};
u32 gStageObjK1C_SeqTbls_27[4] = {
#include "game/008D6DD0.inc"
};
f32 gStageObjK1C_ScaleCurves_87[12] = {
#include "game/008D6DE0.inc"
};
u8 gStageObjK1C_ColorKeys_87[32] = {
#include "game/008D6E10.inc"
};
u32 gStageObjK1C_DrawTbls_28[2] = {
#include "game/008D6E30.inc"
};
u32 gStageObjK1C_SeqTbls_28[2] = {
#include "game/008D6E38.inc"
};
f32 gStageObjK1C_ScaleCurves_90[12] = {
#include "game/008D6E40.inc"
};
f32 gStageObjK1C_ScaleCurves_91[16] = {
#include "game/008D6E70.inc"
};
f32 gStageObjK1C_ScaleCurves_92[12] = {
#include "game/008D6EB0.inc"
};
f32 gStageObjK1C_ScaleCurves_93[12] = {
#include "game/008D6EE0.inc"
};
u8 gStageObjK1C_ColorKeys_90[32] = {
#include "game/008D6F10.inc"
};
u8 gStageObjK1C_ColorKeys_91[32] = {
#include "game/008D6F30.inc"
};
u8 gStageObjK1C_ColorKeys_92[48] = {
#include "game/008D6F50.inc"
};
u8 gStageObjK1C_ColorKeys_93[32] = {
#include "game/008D6F80.inc"
};
u32 gStageObjK1C_DrawTbls_29[2] = {
#include "game/008D6FA0.inc"
};
u16 MonsterFx_TescaAuraFrames[4] = {
#include "game/008D6FA8.inc"
};
u8 MonsterFx_TescaAuraKinds[16] = {
#include "game/008D6FB0.inc"
};
u16 MonsterFx_TescaAuraStarts[16] = {
#include "game/008D6FC0.inc"
};
u32 gStageObjK1C_DrawTbls_30[4] = {
#include "game/008D6FE0.inc"
};
u32 MonsterFx_RangedSplashDelays[4] = {
#include "game/008D6FF0.inc"
};
u32 MonsterFx_RangedSplashFrames[4] = {
#include "game/008D7000.inc"
};
f32 gStageObjK1C_ScaleCurves_94[12] = {
#include "game/008D7010.inc"
};
f32 gStageObjK1C_ScaleCurves_96[16] = {
#include "game/008D7040.inc"
};
f32 gStageObjK1C_ScaleCurves_98[16] = {
#include "game/008D7080.inc"
};
u8 gStageObjK1C_ColorKeys_94[32] = {
#include "game/008D70C0.inc"
};
u8 gStageObjK1C_ColorKeys_96[32] = {
#include "game/008D70E0.inc"
};
u8 gStageObjK1C_ColorKeys_98[48] = {
#include "game/008D7100.inc"
};
f32 gStageObjK1C_ScaleCurves_101[8] = {
#include "game/008D7130.inc"
};
u8 gStageObjK1C_ColorKeys_101[40] = {
#include "game/008D7150.inc"
};
u32 gStageObjK1C_DrawTbls_31[2] = {
#include "game/008D7178.inc"
};
u32 gStageObjK1C_SeqTbls_31[4] = {
#include "game/008D7180.inc"
};
f32 gStageObjK1C_ScaleCurves_102[16] = {
#include "game/008D7190.inc"
};
f32 gStageObjK1C_ScaleCurves_103[16] = {
#include "game/008D71D0.inc"
};
f32 gStageObjK1C_ScaleCurves_104[12] = {
#include "game/008D7210.inc"
};
f32 gStageObjK1C_ScaleCurves_105[16] = {
#include "game/008D7240.inc"
};
u8 gStageObjK1C_ColorKeys_102[32] = {
#include "game/008D7280.inc"
};
u8 gStageObjK1C_ColorKeys_103[32] = {
#include "game/008D72A0.inc"
};
u8 gStageObjK1C_ColorKeys_104[32] = {
#include "game/008D72C0.inc"
};
u8 gStageObjK1C_ColorKeys_105[32] = {
#include "game/008D72E0.inc"
};
u32 gStageObjK1C_DrawTbls_32[2] = {
#include "game/008D7300.inc"
};
u32 MonsterFx_ChameleosImpactFrames[2] = {
#include "game/008D7308.inc"
};
u32 MonsterFx_ChameleosImpactKinds[8] = {
#include "game/008D7310.inc"
};
f32 gStageObjK1C_ScaleCurves_106[12] = {
#include "game/008D7330.inc"
};
f32 gStageObjK1C_ScaleCurves_107[12] = {
#include "game/008D7360.inc"
};
f32 gStageObjK1C_ScaleCurves_108[8] = {
#include "game/008D7390.inc"
};
u8 gStageObjK1C_ColorKeys_106[48] = {
#include "game/008D73B0.inc"
};
u8 gStageObjK1C_ColorKeys_107[48] = {
#include "game/008D73E0.inc"
};
u8 gStageObjK1C_ColorKeys_108[24] = {
#include "game/008D7410.inc"
};
u32 gStageObjK1C_DrawTbls_33[2] = {
#include "game/008D7428.inc"
};
u32 MonsterFx_ChameleosGroundBurstFrames[2] = {
#include "game/008D7430.inc"
};
u32 MonsterFx_ChameleosGroundBurstKinds[2] = {
#include "game/008D7438.inc"
};
u32 MonsterFx_ChameleosGroundBurstDelays[4] = {
#include "game/008D7440.inc"
};
f32 gStageObjK1C_ScaleCurves_109[12] = {
#include "game/008D7450.inc"
};
u8 gStageObjK1C_ColorKeys_109[32] = {
#include "game/008D7480.inc"
};
u32 gStageObjK1C_DrawTbls_34[2] = {
#include "game/008D74A0.inc"
};
u32 gStageObjK1C_SeqTbls_34[2] = {
#include "game/008D74A8.inc"
};
f32 gStageObjK1C_ScaleCurves_110[12] = {
#include "game/008D74B0.inc"
};
u8 gStageObjK1C_ColorKeys_110[24] = {
#include "game/008D74E0.inc"
};
u32 gStageObjK1C_DrawTbls_35[2] = {
#include "game/008D74F8.inc"
};
f32 gStageObjK1C_ScaleCurves_111[12] = {
#include "game/008D7500.inc"
};
u8 gStageObjK1C_ColorKeys_111[32] = {
#include "game/008D7530.inc"
};
u32 gStageObjK1C_DrawTbls_36[2] = {
#include "game/008D7550.inc"
};
u32 gStageObjK1C_SeqTbls_36[2] = {
#include "game/008D7558.inc"
};
f32 gStageObjK1C_ScaleCurves_112[8] = {
#include "game/008D7560.inc"
};
f32 gStageObjK1C_ScaleCurves_113[12] = {
#include "game/008D7580.inc"
};
f32 gStageObjK1C_ScaleCurves_114[12] = {
#include "game/008D75B0.inc"
};
u8 gStageObjK1C_ColorKeys_112[32] = {
#include "game/008D75E0.inc"
};
u8 gStageObjK1C_ColorKeys_113[32] = {
#include "game/008D7600.inc"
};
u8 gStageObjK1C_ColorKeys_114[24] = {
#include "game/008D7620.inc"
};
u32 gStageObjK1C_DrawTbls_37[2] = {
#include "game/008D7638.inc"
};
u32 gStageObjK1C_SeqTbls_37[4] = {
#include "game/008D7640.inc"
};
f32 gStageObjK1C_ScaleCurves_115[8] = {
#include "game/008D7650.inc"
};
f32 gStageObjK1C_ScaleCurves_116[12] = {
#include "game/008D7670.inc"
};
f32 gStageObjK1C_ScaleCurves_117[12] = {
#include "game/008D76A0.inc"
};
u8 gStageObjK1C_ColorKeys_115[32] = {
#include "game/008D76D0.inc"
};
u8 gStageObjK1C_ColorKeys_116[48] = {
#include "game/008D76F0.inc"
};
u8 gStageObjK1C_ColorKeys_117[40] = {
#include "game/008D7720.inc"
};
u32 gStageObjK1C_DrawTbls_38[2] = {
#include "game/008D7748.inc"
};
u32 gStageObjK1C_SeqTbls_38[2] = {
#include "game/008D7750.inc"
};
u32 MonsterFx_FatalisFireballDelays[2] = {
#include "game/008D7758.inc"
};
f32 gStageObjK1C_ScaleCurves_118[8] = {
#include "game/008D7760.inc"
};
u8 gStageObjK1C_ColorKeys_118[40] = {
#include "game/008D7780.inc"
};
u32 gStageObjK1C_DrawTbls_39[2] = {
#include "game/008D77A8.inc"
};
u32 gStageObjK1C_SeqTbls_39[4] = {
#include "game/008D77B0.inc"
};
f32 gStageObjK1C_ScaleCurves_120[16] = {
#include "game/008D77C0.inc"
};
u8 gStageObjK1C_ColorKeys_120[32] = {
#include "game/008D7800.inc"
};
u32 gStageObjK1C_DrawTbls_40[2] = {
#include "game/008D7820.inc"
};
u32 gStageObjK1C_SeqTbls_40[2] = {
#include "game/008D7828.inc"
};
f32 *gStageObjK1C_ScaleCurves[124] = {
    gStageObjK1C_ScaleCurves_0, gStageObjK1C_ScaleCurves_1, gStageObjK1C_ScaleCurves_2, gStageObjK1C_ScaleCurves_3,
    gStageObjK1C_ScaleCurves_4, gStageObjK1C_ScaleCurves_5, gStageObjK1C_ScaleCurves_5, gStageObjK1C_ScaleCurves_7,
    gStageObjK1C_ScaleCurves_7, gStageObjK1C_ScaleCurves_9, gStageObjK1C_ScaleCurves_9, gStageObjK1C_ScaleCurves_9,
    gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_15,
    gStageObjK1C_ScaleCurves_16, gStageObjK1C_ScaleCurves_17, gStageObjK1C_ScaleCurves_18, gStageObjK1C_ScaleCurves_19,
    gStageObjK1C_ScaleCurves_20, gStageObjK1C_ScaleCurves_21, gStageObjK1C_ScaleCurves_21, gStageObjK1C_ScaleCurves_21,
    gStageObjK1C_ScaleCurves_24, gStageObjK1C_ScaleCurves_25, gStageObjK1C_ScaleCurves_25, gStageObjK1C_ScaleCurves_27,
    gStageObjK1C_ScaleCurves_28, gStageObjK1C_ScaleCurves_28, gStageObjK1C_ScaleCurves_30, gStageObjK1C_ScaleCurves_31,
    gStageObjK1C_ScaleCurves_32, gStageObjK1C_ScaleCurves_32, gStageObjK1C_ScaleCurves_34, gStageObjK1C_ScaleCurves_35,
    gStageObjK1C_ScaleCurves_35, gStageObjK1C_ScaleCurves_35, gStageObjK1C_ScaleCurves_38, gStageObjK1C_ScaleCurves_39,
    gStageObjK1C_ScaleCurves_40, gStageObjK1C_ScaleCurves_41, gStageObjK1C_ScaleCurves_39, gStageObjK1C_ScaleCurves_43,
    gStageObjK1C_ScaleCurves_44, gStageObjK1C_ScaleCurves_45, gStageObjK1C_ScaleCurves_46, gStageObjK1C_ScaleCurves_47,
    gStageObjK1C_ScaleCurves_47, gStageObjK1C_ScaleCurves_49, gStageObjK1C_ScaleCurves_50, gStageObjK1C_ScaleCurves_50,
    gStageObjK1C_ScaleCurves_52, gStageObjK1C_ScaleCurves_53, gStageObjK1C_ScaleCurves_53, gStageObjK1C_ScaleCurves_55,
    gStageObjK1C_ScaleCurves_56, gStageObjK1C_ScaleCurves_56, gStageObjK1C_ScaleCurves_58, gStageObjK1C_ScaleCurves_59,
    gStageObjK1C_ScaleCurves_60, gStageObjK1C_ScaleCurves_60, gStageObjK1C_ScaleCurves_62, gStageObjK1C_ScaleCurves_63,
    gStageObjK1C_ScaleCurves_64, gStageObjK1C_ScaleCurves_65, gStageObjK1C_ScaleCurves_66, gStageObjK1C_ScaleCurves_66,
    gStageObjK1C_ScaleCurves_68, gStageObjK1C_ScaleCurves_68, gStageObjK1C_ScaleCurves_70, gStageObjK1C_ScaleCurves_71,
    gStageObjK1C_ScaleCurves_72, gStageObjK1C_ScaleCurves_73, gStageObjK1C_ScaleCurves_74, gStageObjK1C_ScaleCurves_74,
    gStageObjK1C_ScaleCurves_76, gStageObjK1C_ScaleCurves_77, gStageObjK1C_ScaleCurves_78, gStageObjK1C_ScaleCurves_79,
    gStageObjK1C_ScaleCurves_80, gStageObjK1C_ScaleCurves_81, gStageObjK1C_ScaleCurves_82, gStageObjK1C_ScaleCurves_83,
    gStageObjK1C_ScaleCurves_84, gStageObjK1C_ScaleCurves_85, gStageObjK1C_ScaleCurves_86, gStageObjK1C_ScaleCurves_87,
    gStageObjK1C_ScaleCurves_87, gStageObjK1C_ScaleCurves_87, gStageObjK1C_ScaleCurves_90, gStageObjK1C_ScaleCurves_91,
    gStageObjK1C_ScaleCurves_92, gStageObjK1C_ScaleCurves_93, gStageObjK1C_ScaleCurves_94, gStageObjK1C_ScaleCurves_94,
    gStageObjK1C_ScaleCurves_96, gStageObjK1C_ScaleCurves_96, gStageObjK1C_ScaleCurves_98, gStageObjK1C_ScaleCurves_98,
    gStageObjK1C_ScaleCurves_98, gStageObjK1C_ScaleCurves_101, gStageObjK1C_ScaleCurves_102, gStageObjK1C_ScaleCurves_103,
    gStageObjK1C_ScaleCurves_104, gStageObjK1C_ScaleCurves_105, gStageObjK1C_ScaleCurves_106, gStageObjK1C_ScaleCurves_107,
    gStageObjK1C_ScaleCurves_108, gStageObjK1C_ScaleCurves_109, gStageObjK1C_ScaleCurves_110, gStageObjK1C_ScaleCurves_111,
    gStageObjK1C_ScaleCurves_112, gStageObjK1C_ScaleCurves_113, gStageObjK1C_ScaleCurves_114, gStageObjK1C_ScaleCurves_115,
    gStageObjK1C_ScaleCurves_116, gStageObjK1C_ScaleCurves_117, gStageObjK1C_ScaleCurves_118, gStageObjK1C_ScaleCurves_118,
    gStageObjK1C_ScaleCurves_120, gStageObjK1C_ScaleCurves_120, NULL, NULL,
};
u8 *gStageObjK1C_ColorKeys[124] = {
    gStageObjK1C_ColorKeys_0, gStageObjK1C_ColorKeys_1, gStageObjK1C_ColorKeys_1, gStageObjK1C_ColorKeys_3,
    gStageObjK1C_ColorKeys_3, gStageObjK1C_ColorKeys_5, gStageObjK1C_ColorKeys_5, gStageObjK1C_ColorKeys_5,
    gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_11,
    gStageObjK1C_ColorKeys_12, gStageObjK1C_ColorKeys_13, gStageObjK1C_ColorKeys_14, gStageObjK1C_ColorKeys_15,
    gStageObjK1C_ColorKeys_16, gStageObjK1C_ColorKeys_17, gStageObjK1C_ColorKeys_17, gStageObjK1C_ColorKeys_17,
    gStageObjK1C_ColorKeys_20, gStageObjK1C_ColorKeys_21, gStageObjK1C_ColorKeys_21, gStageObjK1C_ColorKeys_23,
    gStageObjK1C_ColorKeys_24, gStageObjK1C_ColorKeys_24, gStageObjK1C_ColorKeys_26, gStageObjK1C_ColorKeys_27,
    gStageObjK1C_ColorKeys_28, gStageObjK1C_ColorKeys_28, gStageObjK1C_ColorKeys_30, gStageObjK1C_ColorKeys_31,
    gStageObjK1C_ColorKeys_31, gStageObjK1C_ColorKeys_31, gStageObjK1C_ColorKeys_34, gStageObjK1C_ColorKeys_35,
    gStageObjK1C_ColorKeys_36, gStageObjK1C_ColorKeys_37, gStageObjK1C_ColorKeys_35, gStageObjK1C_ColorKeys_39,
    gStageObjK1C_ColorKeys_40, gStageObjK1C_ColorKeys_41, gStageObjK1C_ColorKeys_42, gStageObjK1C_ColorKeys_43,
    gStageObjK1C_ColorKeys_44, gStageObjK1C_ColorKeys_44, gStageObjK1C_ColorKeys_46, gStageObjK1C_ColorKeys_47,
    gStageObjK1C_ColorKeys_47, gStageObjK1C_ColorKeys_49, gStageObjK1C_ColorKeys_50, gStageObjK1C_ColorKeys_50,
    gStageObjK1C_ColorKeys_52, gStageObjK1C_ColorKeys_53, gStageObjK1C_ColorKeys_53, gStageObjK1C_ColorKeys_55,
    gStageObjK1C_ColorKeys_56, gStageObjK1C_ColorKeys_57, gStageObjK1C_ColorKeys_57, gStageObjK1C_ColorKeys_59,
    gStageObjK1C_ColorKeys_60, gStageObjK1C_ColorKeys_61, gStageObjK1C_ColorKeys_62, gStageObjK1C_ColorKeys_63,
    gStageObjK1C_ColorKeys_63, gStageObjK1C_ColorKeys_65, gStageObjK1C_ColorKeys_65, gStageObjK1C_ColorKeys_67,
    gStageObjK1C_ColorKeys_68, gStageObjK1C_ColorKeys_69, gStageObjK1C_ColorKeys_70, gStageObjK1C_ColorKeys_71,
    gStageObjK1C_ColorKeys_71, gStageObjK1C_ColorKeys_73, gStageObjK1C_ColorKeys_74, gStageObjK1C_ColorKeys_75,
    gStageObjK1C_ColorKeys_76, gStageObjK1C_ColorKeys_77, gStageObjK1C_ColorKeys_78, gStageObjK1C_ColorKeys_79,
    gStageObjK1C_ColorKeys_80, gStageObjK1C_ColorKeys_81, gStageObjK1C_ColorKeys_82, gStageObjK1C_ColorKeys_83,
    gStageObjK1C_ColorKeys_84, gStageObjK1C_ColorKeys_85, gStageObjK1C_ColorKeys_86, gStageObjK1C_ColorKeys_87,
    gStageObjK1C_ColorKeys_87, gStageObjK1C_ColorKeys_87, gStageObjK1C_ColorKeys_90, gStageObjK1C_ColorKeys_91,
    gStageObjK1C_ColorKeys_92, gStageObjK1C_ColorKeys_93, gStageObjK1C_ColorKeys_94, gStageObjK1C_ColorKeys_94,
    gStageObjK1C_ColorKeys_96, gStageObjK1C_ColorKeys_96, gStageObjK1C_ColorKeys_98, gStageObjK1C_ColorKeys_98,
    gStageObjK1C_ColorKeys_98, gStageObjK1C_ColorKeys_101, gStageObjK1C_ColorKeys_102, gStageObjK1C_ColorKeys_103,
    gStageObjK1C_ColorKeys_104, gStageObjK1C_ColorKeys_105, gStageObjK1C_ColorKeys_106, gStageObjK1C_ColorKeys_107,
    gStageObjK1C_ColorKeys_108, gStageObjK1C_ColorKeys_109, gStageObjK1C_ColorKeys_110, gStageObjK1C_ColorKeys_111,
    gStageObjK1C_ColorKeys_112, gStageObjK1C_ColorKeys_113, gStageObjK1C_ColorKeys_114, gStageObjK1C_ColorKeys_115,
    gStageObjK1C_ColorKeys_116, gStageObjK1C_ColorKeys_117, gStageObjK1C_ColorKeys_118, gStageObjK1C_ColorKeys_118,
    gStageObjK1C_ColorKeys_120, gStageObjK1C_ColorKeys_120, NULL, NULL,
};
s16 gStageObjK1C_Counts[48] = {
#include "game/008D7C10.inc"
};
s16 gStageObjK1C_Limits[48] = {
#include "game/008D7C70.inc"
};
s16 gStageObjK1C_ScaleCurveIdx[48] = {
#include "game/008D7CD0.inc"
};
s16 gStageObjK1C_ColorCurveIdx[48] = {
#include "game/008D7D30.inc"
};
u32 gStageObjK1C_DrawTbls[44] = {
    0, (u32)gStageObjK1C_DrawTbls_1, (u32)gStageObjK1C_DrawTbls_2, (u32)gStageObjK1C_DrawTbls_3,
    (u32)gStageObjK1C_DrawTbls_4, (u32)gStageObjK1C_DrawTbls_5, (u32)gStageObjK1C_DrawTbls_6, (u32)gStageObjK1C_DrawTbls_7,
    (u32)gStageObjK1C_DrawTbls_8, (u32)gStageObjK1C_DrawTbls_9, (u32)gStageObjK1C_DrawTbls_10, 0,
    (u32)gStageObjK1C_DrawTbls_12, (u32)gStageObjK1C_DrawTbls_13, (u32)gStageObjK1C_DrawTbls_14, (u32)gStageObjK1C_DrawTbls_15,
    (u32)gStageObjK1C_DrawTbls_16, (u32)gStageObjK1C_DrawTbls_15, (u32)gStageObjK1C_DrawTbls_10, (u32)gStageObjK1C_DrawTbls_19,
    (u32)gStageObjK1C_DrawTbls_20, (u32)gStageObjK1C_DrawTbls_20, (u32)gStageObjK1C_DrawTbls_22, (u32)gStageObjK1C_DrawTbls_23,
    (u32)gStageObjK1C_DrawTbls_24, (u32)gStageObjK1C_DrawTbls_25, (u32)gStageObjK1C_DrawTbls_26, (u32)gStageObjK1C_DrawTbls_27,
    (u32)gStageObjK1C_DrawTbls_28, (u32)gStageObjK1C_DrawTbls_29, (u32)gStageObjK1C_DrawTbls_30, (u32)gStageObjK1C_DrawTbls_31,
    (u32)gStageObjK1C_DrawTbls_32, (u32)gStageObjK1C_DrawTbls_33, (u32)gStageObjK1C_DrawTbls_34, (u32)gStageObjK1C_DrawTbls_35,
    (u32)gStageObjK1C_DrawTbls_36, (u32)gStageObjK1C_DrawTbls_37, (u32)gStageObjK1C_DrawTbls_38, (u32)gStageObjK1C_DrawTbls_39,
    (u32)gStageObjK1C_DrawTbls_40, 0, 0, 0,
};
u32 gStageObjK1C_ModelSetIds[24] = {
#include "game/008D7E40.inc"
};
u32 gStageObjK1C_SeqTbls[42] = {
    0, (u32)gStageObjK1C_SeqTbls_1, (u32)gStageObjK1C_SeqTbls_2, 0,
    0, (u32)gStageObjK1C_SeqTbls_5, (u32)gStageObjK1C_SeqTbls_5, (u32)gStageObjK1C_SeqTbls_7,
    (u32)gStageObjK1C_SeqTbls_8, (u32)gStageObjK1C_SeqTbls_9, (u32)gStageObjK1C_SeqTbls_10, 0,
    (u32)MonsterFx_KushalaRingFrames, (u32)gStageObjK1C_SeqTbls_13, (u32)gStageObjK1C_SeqTbls_14, (u32)MonsterFx_DustBurstFrames,
    (u32)MonsterFx_KushalaGroundWindFrames, (u32)MonsterFx_DustBurstFrames, (u32)gStageObjK1C_SeqTbls_10, (u32)gStageObjK1C_SeqTbls_19,
    (u32)gStageObjK1C_SeqTbls_20, (u32)gStageObjK1C_SeqTbls_20, (u32)gStageObjK1C_SeqTbls_22, (u32)gStageObjK1C_SeqTbls_23,
    (u32)gStageObjK1C_SeqTbls_24, (u32)gStageObjK1C_SeqTbls_25, (u32)gStageObjK1C_SeqTbls_26, (u32)gStageObjK1C_SeqTbls_27,
    (u32)gStageObjK1C_SeqTbls_28, (u32)MonsterFx_TescaAuraFrames, (u32)MonsterFx_RangedSplashFrames, (u32)gStageObjK1C_SeqTbls_31,
    (u32)MonsterFx_ChameleosImpactFrames, (u32)MonsterFx_ChameleosGroundBurstFrames, (u32)gStageObjK1C_SeqTbls_34, 0,
    (u32)gStageObjK1C_SeqTbls_36, (u32)gStageObjK1C_SeqTbls_37, (u32)gStageObjK1C_SeqTbls_38, (u32)gStageObjK1C_SeqTbls_39,
    (u32)gStageObjK1C_SeqTbls_40, 0,
};
u8 gStageObjK1C_ModelSets_0[8] = {
#include "game/008D7F48.inc"
};
u8 gStageObjK1C_ModelSets_1[8] = {
#include "game/008D7F50.inc"
};
u8 gStageObjK1C_ModelSets_2[8] = {
#include "game/008D7F58.inc"
};
u8 gStageObjK1C_ModelSets_3[8] = {
#include "game/008D7F60.inc"
};
u8 gStageObjK1C_ModelSets_4[8] = {
#include "game/008D7F68.inc"
};
u8 gStageObjK1C_ModelSets_5[8] = {
#include "game/008D7F70.inc"
};
u8 gStageObjK1C_ModelSets_6[8] = {
#include "game/008D7F78.inc"
};
u8 gStageObjK1C_ModelSets_7[8] = {
#include "game/008D7F80.inc"
};
u8 gStageObjK1C_ModelSets_8[8] = {
#include "game/008D7F88.inc"
};
u8 gStageObjK1C_ModelSets_9[16] = {
#include "game/008D7F90.inc"
};
u8 *gStageObjK1C_ModelSets[12] = {
    gStageObjK1C_ModelSets_0, gStageObjK1C_ModelSets_1, gStageObjK1C_ModelSets_2, gStageObjK1C_ModelSets_3,
    gStageObjK1C_ModelSets_4, gStageObjK1C_ModelSets_5, gStageObjK1C_ModelSets_6, gStageObjK1C_ModelSets_7,
    gStageObjK1C_ModelSets_8, gStageObjK1C_ModelSets_9, NULL, NULL,
};
f32 gAreaFlyerCenters[65][3] = {
#include "game/008D7FD0.inc"
};
f32 gAreaFlyerScaleKeys[8] = {
#include "game/008D82E0.inc"
};
u8 gAreaFlyerColorKeys[32] = {
#include "game/008D8300.inc"
};
u32 GunFx_ScaleKeyTbl_0[20] = {
#include "game/008D8320.inc"
};
u32 GunFx_ScaleKeyTbl_1[20] = {
#include "game/008D8370.inc"
};
u32 GunFx_ScaleKeyTbl_2[12] = {
#include "game/008D83C0.inc"
};
u32 GunFx_ColorKeyTbl_0[8] = {
#include "game/008D83F0.inc"
};
u32 GunFx_ColorKeyTbl_1[8] = {
#include "game/008D8410.inc"
};
u32 GunFx_ColorKeyTbl_2[6] = {
#include "game/008D8430.inc"
};
u32 GunFx_ModelIds[2] = {
#include "game/008D8448.inc"
};
u32 GunFx_ScaleKeyTbl[4] = {
    (u32)GunFx_ScaleKeyTbl_0, (u32)GunFx_ScaleKeyTbl_1, (u32)GunFx_ScaleKeyTbl_2, 0,
};
u32 GunFx_ColorKeyTbl[4] = {
    (u32)GunFx_ColorKeyTbl_0, (u32)GunFx_ColorKeyTbl_1, (u32)GunFx_ColorKeyTbl_2, 0,
};
u32 GunFx_SmokePuffScaleKeys[12] = {
#include "game/008D8470.inc"
};
u32 GunFx_SmokePuffColorKeys[12] = {
#include "game/008D84A0.inc"
};
u32 GunFx_SmokeBlastScaleKeys[12] = {
#include "game/008D84D0.inc"
};
u32 GunFx_SmokeBlastColorKeys[8] = {
#include "game/008D8500.inc"
};
u32 GunFx_SmokeCloudScaleKeys[12] = {
#include "game/008D8520.inc"
};
u32 GunFx_SmokeCloudColorKeys[6] = {
#include "game/008D8550.inc"
};
u32 GunFx_Vars01_0[3] = {
#include "game/008D8568.inc"
};
u32 GunFx_Vars01_1[12] = {
#include "game/008D8580.inc"
};
u32 GunFx_Vars23_0[8] = {
#include "game/008D85B0.inc"
};
u32 GunFx_Vars23_1[12] = {
#include "game/008D85D0.inc"
};
u32 GunFx_SmokeDelaysLow[2] = {
    (u32)GunFx_Vars01_0, (u32)GunFx_Vars01_1,
};
u32 GunFx_SmokeDelaysHigh[2] = {
    (u32)GunFx_Vars23_0, (u32)GunFx_Vars23_1,
};
u32 GunFx_Pos01_0[12] = {
#include "game/008D8610.inc"
};
u32 GunFx_Pos01_1[10] = {
#include "game/008D8640.inc"
};
u32 GunFx_Pos23_0[3] = {
#include "game/008D8668.inc"
};
u32 GunFx_Pos23_1[24] = {
#include "game/008D8680.inc"
};
u32 GunFx_SmokeOffsetsLow[2] = {
    (u32)GunFx_Pos01_0, (u32)GunFx_Pos01_1,
};
u32 GunFx_SmokeOffsetsHigh[2] = {
    (u32)GunFx_Pos23_0, (u32)GunFx_Pos23_1,
};
u32 GunFx_Scale01_0[4] = {
#include "game/008D86F0.inc"
};
u32 GunFx_Scale01_1[4] = {
#include "game/008D8700.inc"
};
u32 GunFx_Scale23_0[4] = {
#include "game/008D8710.inc"
};
u32 GunFx_Scale23_1[8] = {
#include "game/008D8720.inc"
};
u32 GunFx_SmokeSizesLow[2] = {
    (u32)GunFx_Scale01_0, (u32)GunFx_Scale01_1,
};
u32 GunFx_SmokeSizesHigh[2] = {
    (u32)GunFx_Scale23_0, (u32)GunFx_Scale23_1,
};
u32 gGunFx_ChargePartFrames[2] = {
#include "game/008D8750.inc"
};
u32 GunFx_ChargeDelays[2] = {
#include "game/008D8758.inc"
};
u32 GunFx_ChargeTipSizes[4] = {
#include "game/008D8760.inc"
};
u32 GunFx_ChargeTipScaleKeys[16] = {
#include "game/008D8770.inc"
};
u32 GunFx_ChargeTipColorKeys[8] = {
#include "game/008D87B0.inc"
};
u32 GunFx_ChargeGlowScaleKeys[12] = {
#include "game/008D87D0.inc"
};
u32 GunFx_ChargeGlowColorKeys[8] = {
#include "game/008D8800.inc"
};
u32 GunFx_ChargeFlameScaleKeys[12] = {
#include "game/008D8820.inc"
};
u32 GunFx_ChargeFlameColorKeys[8] = {
#include "game/008D8850.inc"
};
u32 GunFx_ChargeJetScaleKeys[12] = {
#include "game/008D8870.inc"
};
u32 GunFx_ChargeJetColorKeys[6] = {
#include "game/008D88A0.inc"
};
u32 GunFx_SprayStreamLengths[2] = {
#include "game/008D88B8.inc"
};
u32 GunFx_SprayStreamSpreads[4] = {
#include "game/008D88C0.inc"
};
u32 GunFx_SprayCloudSpreads[6] = {
#include "game/008D88D0.inc"
};
u32 GunFx_SprayDropLengths[2] = {
#include "game/008D88E8.inc"
};
u32 GunFx_SprayStreamStarts[2] = {
#include "game/008D88F0.inc"
};
u32 GunFx_SprayPartFrames[2] = {
#include "game/008D88F8.inc"
};
u32 GunFx_SizeScaleKeys1_0[12] = {
#include "game/008D8900.inc"
};
u32 GunFx_SizeColorKeys1_0[8] = {
#include "game/008D8930.inc"
};
u32 GunFx_SizeScaleKeys2_0[16] = {
#include "game/008D8950.inc"
};
u32 GunFx_SizeColorKeys2_0[8] = {
#include "game/008D8990.inc"
};
u32 GunFx_SizeScaleKeys3_0[12] = {
#include "game/008D89B0.inc"
};
u32 GunFx_SizeColorKeys3_0[8] = {
#include "game/008D89E0.inc"
};
u32 GunFx_SizeScaleKeys1_1[12] = {
#include "game/008D8A00.inc"
};
u32 GunFx_SizeColorKeys1_1[8] = {
#include "game/008D8A30.inc"
};
u32 GunFx_SizeScaleKeys2_1[16] = {
#include "game/008D8A50.inc"
};
u32 GunFx_SizeColorKeys2_1[8] = {
#include "game/008D8A90.inc"
};
u32 GunFx_SizeScaleKeys3_1[12] = {
#include "game/008D8AB0.inc"
};
u32 GunFx_SizeColorKeys3_1[6] = {
#include "game/008D8AE0.inc"
};
u32 D_008D8AF8[2] = {
#include "game/008D8AF8.inc"
};
u32 GunFx_SprayStreamScaleKeys[2] = {
    (u32)GunFx_SizeScaleKeys1_0, (u32)GunFx_SizeScaleKeys1_1,
};
u32 GunFx_SprayStreamColorKeys[2] = {
    (u32)GunFx_SizeColorKeys1_0, (u32)GunFx_SizeColorKeys1_1,
};
u32 GunFx_SprayCloudScaleKeys[2] = {
    (u32)GunFx_SizeScaleKeys2_0, (u32)GunFx_SizeScaleKeys2_1,
};
u32 GunFx_SprayCloudColorKeys[2] = {
    (u32)GunFx_SizeColorKeys2_0, (u32)GunFx_SizeColorKeys2_1,
};
u32 GunFx_SprayDropScaleKeys[2] = {
    (u32)GunFx_SizeScaleKeys3_0, (u32)GunFx_SizeScaleKeys3_1,
};
u32 GunFx_SprayDropColorKeys[2] = {
    (u32)GunFx_SizeColorKeys3_0, (u32)GunFx_SizeColorKeys3_1,
};
u32 GunFx_BarrelSmokeDelays[2] = {
#include "game/008D8B30.inc"
};
u32 GunFx_BarrelSmokeSizes[2] = {
#include "game/008D8B38.inc"
};
u32 GunFx_BarrelSmokeScaleKeys[12] = {
#include "game/008D8B40.inc"
};
u32 GunFx_BarrelSmokeColorKeys[8] = {
#include "game/008D8B70.inc"
};
