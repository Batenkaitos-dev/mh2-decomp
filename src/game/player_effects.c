/*
 * The effects of five kinds of stage object, each run by its Update (Setup,
 * Run, End, Free) and drawn by its Draw callback from draw slots:
 *
 * - GunFx (kind 0x1F): the gunlance's shot flash and smoke, the bowgun's and
 * gunlance's spent casings, the wyvern fire's charge and puffs, and a lasting
 * effect on Daimyo Hermitaur and Shogun Ceanataur. GunFx_Setup sets the parts
 * up and GunFx_Run moves them; this file holds the effect's end, draw and
 * creators.
 *
 * - VersusBait (kind 0x25): the bait thrown to a defeated monster at a versus
 * match's end.
 *
 * - TeostraBlastCharge (kind 0x27): the gusts around Lunastra or Teostra
 * while it charges its blast.
 *
 * - PlayerFx (kind 0x29): seventeen types of sprite bursts for the players'
 * weapon charges and swings, floor hazards, statuses and hits, and for a
 * trap, a bomb's burst and a volcanic vent at a point.
 *
 * - BowArc (kind 0x2B): a bow's aim arc along the arrow's flight, for the
 * local player.
 *
 * Each object keeps its parts in its blocks of the object buffer
 * (StageObj.buf) as GunFxWork, TeostraBlastChargeWork, PlayerFxPart or BowArcWork.
 * The weapon states, the monsters' states, the hit effects and the player's
 * update create them.
 *
 * The file ends with the kinds' tables and key curves, then key curves that
 * the weapon states' tables list.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "stageobj.h"
#include "lobby_vobj.h"
/* Declarations of the functions called below. */
s32 Obj_InCurrentArea(void *obj);
void Vec_Copy(f32 *out, f32 *in);
void DrawSlotB_Free(s32);

/* a part of a gun effect (0x20 bytes, in its blocks) */
typedef struct GunFxPart {
    s16 slot;
    s8 t;                       /* 0x02: its frame, negative: not shown yet */
    s8 frame;                   /* 0x03: a texture frame (4 x 4) */
    s32 color;                  /* 0x04 */
    f32 sc[3];                  /* 0x08: its scale */
    DrawSlot *node;                /* 0x14 */
    u8 size[0x1E - 0x18];
    s16 angle;                  /* 0x1E: its z angle */
} GunFxPart;

/* the work block of a gun effect */
typedef struct GunFxWork {
    f32 m[16];                  /* 0x000: the owner's bone 0xE (for type 5 the monster's bone 0x1F): type 0's part 1 and type 4's part 0 are drawn in it */
    f32 scale;                  /* 0x040 */
    GunFxPart part[11];         /* 0x044 */
    u8 count;                   /* 0x1A4: parts in use */
    u8 unk1A5[0x1B7 - 0x1A5];
    u8 sprayLevel;              /* 0x1B7: type 5: the row of the size tables, 0 or 1; 0 draws with blend bit 2 */
    s32 angle[3];               /* 0x1B8: types 2 and 3 */
    s16 slot[3];                /* 0x1C4: type 3 */
    u8 unk1CA[2];
    DrawSlot *node[3];             /* 0x1CC */
} GunFxWork;

/* a part of a player effect (0x3C bytes, in its blocks) */
typedef struct PlayerFxPart {
    s16 slot;                   /* 0x00: its draw slot */
    u8 type;                    /* 0x02: its part type, the index into its effect type's tables */
    u8 variant;
    f32 pos[3];                 /* 0x04 */
    u32 color;                  /* 0x10 */
    f32 sc[3];                  /* 0x14: times scale */
    f32 scale;                  /* 0x20: the object scale times the type factor */
    DrawSlot *node;                /* 0x24 */
    s16 frame;                  /* 0x28 */
    u16 rotX;                  /* 0x2A: the x angle (16-bit) */
    u16 rotY;                  /* 0x2C: the y angle; 0x8000 turns every odd part of some types */
    u16 rotZ;                  /* 0x2E: the z angle */
    s16 spinX;                  /* 0x30 */
    s16 spinY;                  /* 0x32 */
    s16 spinZ;                  /* 0x34: added to rotZ each frame */
    u8 useColorKeys;                   /* 0x36 */
    u8 scale2D;                   /* 0x37 */
    f32 speed;                  /* 0x38 */
} PlayerFxPart;

/* a part of a blast charge (0x14 bytes, ten in its blocks): gusts 0 to 5, puffs 6 to 9 */
typedef struct TeostraBlastChargePart {
    s16 slot;                   /* 0x00: its draw slot */
    s8 t;                       /* 0x02: its frame, negative: a delay */
    s8 life;                    /* 0x03: puffs: the frame from which they rise */
    DrawSlot *node;                /* 0x04 */
    s16 rotX;                  /* 0x08: the x angle */
    s16 rotY;                  /* 0x0A */
    s16 rotZ;                  /* 0x0C */
    s16 spin;                  /* 0x0E: added to rotY (gusts) or rotZ (puffs) each frame */
    f32 speed;                  /* 0x10 */
} TeostraBlastChargePart;

/* the work block of a blast charge */
typedef struct TeostraBlastChargeWork {
    TeostraBlastChargePart part[10];         /* 0x00 */
    u8 unkC8[0xD0 - 0xC8];
    f32 m[16];                  /* 0xD0: the owner's bone 1 */
} TeostraBlastChargeWork;

/* the work block of a bow arc */
typedef struct BowArcWork {
    s32 yaw;                    /* 0x00: the pitch of the owner's bone 0xE (its x axis) */
    s32 pitch;                  /* 0x04: its yaw */
    s32 unk08;                  /* 0x08 */
    s16 timer;                  /* 0x0C: counts to 24, then phase flips */
    u8 phase;                   /* 0x0E: 0 or 1 (the arc's colour) */
} BowArcWork;

/* prototypes */
/* GunFx_Draw's key curves, models and lifetimes by part */
void Mtx_BillboardScaled(f32 *m, f32 *pos);
u8 Player_InCurrentArea(void *obj);
extern u8 *GunFx_ScaleKeyTbl[];
extern u8 *GunFx_ColorKeyTbl[];
extern u16 GunFx_ModelIds[];
extern s8 gGunFx_ChargePartFrames[];
extern u8 D_008D8AF8[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
extern f32 gCamMatrix[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
extern u8 TeostraBlastCharge_GustScaleKeys[];
extern u8 TeostraBlastCharge_GustColorKeys[];
extern u8 TeostraBlastCharge_PuffScaleKeys[];
extern u8 TeostraBlastCharge_PuffColorKeys[];
void BowArrow_FlightParams(s16 idx, s32 *pos, f32 *a2, s16 *a3);
void Gfx_SetTexFilter(s32);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_Concat3x3(f32 *m, f32 *n);
struct EmBone;
void Bone_UpdateWorld(struct EmBone *b, f32 *m);
void Bone_StoreMatrices(struct EmBone *b);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
f32 Math_Sin(f32 x);
s32 Player_IsLocal(void *obj);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 TeostraBlastCharge_AllGustsDone(StageObj *o);
void TeostraBlastCharge_FreePuffs(StageObj *o);
void Vec_RotateY(f32 *v, f32 angle);
extern Vec3f TeostraBlastCharge_PuffDirs[];
extern u16 VersusBait_SettleAngles[];
void PartFx_SpawnAt(f32 *pos, s32 a, f32 f);
void VersusBait_Move(StageObj *o);
extern f32 VersusBait_HandOffset[];
extern u16 D_008D8BA2;
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Add(f32 *out, f32 *a, f32 *b);
f32 Math_Atan2(f32 y, f32 x);
void VersusBait_Draw(DrawSlot *node);
void TeostraBlastCharge_Draw(DrawSlot *node);
void TeostraBlastCharge_InitGust(TeostraBlastChargePart *p, u8 i);
void Mtx_Copy(f32 *dst, f32 *src);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
extern s16 PlayerFx_BowChargeDurations[];
extern f32 PlayerFx_BowChargeScales[];
extern f32 PlayerFx_GreatSwordChargeScales[];
extern u8 TeostraBlastCharge_MonsterKinds[];
s16 EmSlot_FindKind(s16 k);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void BowArc_Draw(DrawSlot *node);
s32 Bow_ArcVisible(void *p, u16 force);
EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
u16 Rand_Next(s32 stream);
void VersusBait_Setup(StageObj *o);
void VersusBait_Run(StageObj *o);
void VersusBait_End(StageObj *o);
void VersusBait_Free(StageObj *o);
void TeostraBlastCharge_Setup(StageObj *o);
void TeostraBlastCharge_Run(StageObj *o);
void TeostraBlastCharge_End(StageObj *o);
void TeostraBlastCharge_Free(StageObj *o);
void PlayerFx_Setup(StageObj *o);
void PlayerFx_Run(StageObj *o);
void PlayerFx_End(StageObj *o);
void PlayerFx_Free(StageObj *o);
void PlayerFx_CreateOnPlayer(Player *p, u8 k, s32 c, f32 scale);
void PlayerFx_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void BowArc_Setup(StageObj *o);
void BowArc_Run(StageObj *o);
void BowArc_End(StageObj *o);
void BowArc_Free(StageObj *o);
f32 Rand_Unit(void);
f32 Rand_Signed(void);
void DrawSlot_Free(s32);
extern s16 PlayerFx_HornNoteDurations[];
void GunFx_Update(StageObj *o);
extern s16 PlayerFx_PartCounts[];
void Mtx_RotateXZY(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_Billboard(f32 *m, f32 *pos);
void Mtx_TurnX(f32 *m, f32 a);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
extern u16 *PlayerFx_MeshTbl[];
extern f32 gCamYawMtx[];
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
u8 HuntingHorn_PlayingNoteColour(Player *p);
void PlayerFx_Draw(DrawSlot *node);
void PlayerFx_PlayStartSound(StageObj *o);
void PlayerFx_InitBowChargePart(StageObj *o, PlayerFxPart *w, EmBone *bone, s16 i);
void PlayerFx_InitGreatSwordChargePart(StageObj *o, PlayerFxPart *w, EmBone *bone, s16 i);
extern s16 PlayerFx_SlashDurations[];
extern s16 PlayerFx_SlashDelays[];
extern s16 D_008D9008[];
extern u8 PlayerFx_BowChargePartTypes[];
extern u8 PlayerFx_BowShotPartTypes[];
extern u16 PlayerFx_BowShotDelays[];
extern s16 PlayerFx_BowShotDurations[];
extern u8 PlayerFx_TrapPartTypes[];
extern u16 PlayerFx_TrapDelays[];
extern s16 PlayerFx_HornNoteDelays[];
extern u8 PlayerFx_GreatSwordChargePartTypes[];
extern s16 PlayerFx_GreatSwordChargeDurations[];
extern u8 PlayerFx_ImpactMeshes[];
extern f32 PlayerFx_ImpactDepths[];
extern u8 PlayerFx_ImpactPartTypes[];
extern u16 PlayerFx_ImpactDelays[];
extern s16 PlayerFx_ImpactDurations[];
extern u8 PlayerFx_HornPlayPartTypes[];
extern s16 PlayerFx_HornPlayDelays[];
extern u8 PlayerFx_DragonPartTypes[];
extern s16 PlayerFx_DragonDelays[];
extern s16 PlayerFx_BombFlameDelays[];
extern u8 PlayerFx_BombRingSmokePartTypes[];
extern s16 D_008D9E58[];
/* end of prototypes */

/*
 * Ends a gun effect: moves it to Free and frees the draw slots its type holds
 * (the parts' slots; type 2 the object's own slot; type 3 its three casings'
 * slots).
 */
void GunFx_End(StageObj *obj)
{
    GunFxWork *work = (GunFxWork *) obj->buf;
    GunFxPart *p;
    u8 i;

    obj->state++;
    p = work->part;
    switch (obj->type) {
    case 0:
        for (i = 0; i < work->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    case 1:
        for (i = 0; i < work->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    case 2:
        if (obj->node != NULL) {
            DrawSlot_Free(obj->slot);
        }
        break;
    case 3:
        for (i = 0; i < 3; i++) {
            if (work->node[i] != NULL) {
                DrawSlot_Free(work->slot[i]);
            }
        }
        break;
    case 4:
    case 5:
    case 6:
        for (i = 0; i < work->count; i++, p++) {
            if (p->node != NULL) {
                DrawSlot_Free(p->slot);
            }
        }
        break;
    }
}

/* Returns the gun effect to the stage object pool. */
void GunFx_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws one part (node->part) of a gun effect with a mesh of the common model
 * set, by type: 0 the shot flash, scaled and coloured by the part's key
 * curves at o->timer and by work->scale (part 1 drawn in the owner's bone
 * matrix work->m); 1 the smoke and sparks, three groups of parts each shown
 * up to its own last frame (30, 25, 3), the last two with a 4 x 4 texture
 * cell from part->frame; 2 and 3 a casing, mesh 0x97, turned by
 * work->angle[part]; 4 the wyvern fire charge, each part up to its frame in
 * GunFx_ChargeLastFrames; 5 the Hermitaur effect while its owner is in the
 * current area (part 2 spins at each draw, part 3 picks one of
 * GunFx_HermitaurMeshes at random); 6 each part in its first 16 frames. All
 * but the casings are drawn without writing depth.
 */
void GunFx_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    GunFxWork *work = (GunFxWork *) obj->buf;
    GunFxPart *p = &work->part[node->part];
    ModelMat *mats;
    u16 mode = 0;
    u8 meshId;
    Model *set = gStageModelSet;
    ModelMatSet *model;
    u8 *colorKeys;
    f32 lastFrame;
    u8 *scaleKeys;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scale[3];
    f32 rot[3];
    s32 color;

    if (set != NULL && set->active && obj->visible) {
    mats = set->mats;
    switch (obj->type) {
    case 0:
        scaleKeys = GunFx_ScaleKeyTbl[node->part];
        colorKeys = GunFx_ColorKeyTbl[node->part];
        model = &set->matSets[GunFx_ModelIds[node->part]];
        rot[0] = 0.0f;
        if (node->part == 1) {
            rot[1] = 0.5462889f;
        } else {
            rot[1] = 0.0f;
        }
        rot[2] = 0.0f;
        Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, obj->timer);
        scale[0] *= work->scale;
        scale[1] *= work->scale;
        scale[2] *= work->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, mtx);
        if (node->part == 1) {
            Mtx_Concat3x3(mtx, work->m);
            Mtx_BillboardScaled(mtx, node->pos);
        } else {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        Color_EvalKeys((s32 *) colorKeys, obj->timer, (u32 *) &color);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, mtx, color, 2, mats);
        Gs_SetState(0x6C, 1);
        break;
    case 1:
        switch (node->part) {
        case 0:
        case 1:
        case 2:
            meshId = 0x4A;
            lastFrame = 30.0f;
            mode |= 4;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            break;
        case 3:
        case 4:
        case 5:
        case 9:
            lastFrame = 25.0f;
            rot[2] = 0.0f;
            meshId = 0x4E;
            mode |= 2;
            break;
        case 6:
        case 7:
        case 8:
        case 10:
            meshId = 0x5E;
            lastFrame = 3.0f;
            mode |= 2;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            break;
        }
        model = &set->matSets[meshId];
        if (p->t >= 0 && p->t <= lastFrame) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            if ((u32) node->part >= 3 && (u32) node->part < 11) {
                Mtx_Translation(uvMtx, 0.25f * (p->frame & 3), 0.25f * (p->frame / 4), 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
            }
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, mtx, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 2:
    case 3:
        model = &set->matSets[0x97];
        if (obj->type == 3) {
            rot[1] = 0.0f;
            rot[0] = 1.5707964f;
        } else {
            rot[0] = 0.0f;
            rot[1] = 1.5707964f;
        }
        rot[2] = (6.2831855f * work->angle[node->part]) / 65536.0f;
        scale[0] = 1.0f;
        scale[1] = 1.0f;
        scale[2] = 1.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, mtx);
        Effect_DrawModelAdd(model, (f32 *)((s32) mtx), (void *)((s32) mats));
        break;
    case 4:
        if (p->t >= 0 && p->t <= gGunFx_ChargePartFrames[node->part]) {
            switch (node->part) {
            case 0:
                mode |= 2;
                rot[2] = 0.0f;
                meshId = 0x11;
                rot[0] = -0.08726647f;
                rot[1] = 0.5462881f;
                break;
            case 1:
                rot[0] = 0.0f;
                meshId = 0x51;
                mode |= 2;
                rot[1] = 0.0f;
                rot[2] = 0.0f;
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                mode |= 4;
                meshId = 0x4A;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                break;
            case 6:
            case 7:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                mode |= 2;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                meshId = 0x5E;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            }
            model = &set->matSets[meshId];
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, mtx);
            if (node->part == 0) {
                Mtx_Concat3x3(mtx, work->m);
                Mtx_BillboardScaled(mtx, node->pos);
            } else {
                Mtx_Concat3x3(mtx, gCamMatrix);
            }
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, mtx, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 5:
        if (Player_InCurrentArea(obj->owner) & 0xFF) {
            switch (node->part) {
            case 0:
            case 1:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                meshId = 0x3E;
                break;
            case 2:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                meshId = 0x3E;
                p->angle += 0x1555;
                break;
            case 3:
                rot[0] = 0.0f;
                rot[1] = 0.0f;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                meshId = D_008D8AF8[Rand_Next(1) % 2];
                break;
            }
            model = &set->matSets[meshId];
            if (work->sprayLevel == 0) {
                mode |= 2;
            }
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, mtx, p->color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    case 6:
        if (p->t >= 0 && p->t < 16) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            model = &set->matSets[0x4A];
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 6, mtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, mtx, p->color, 4, mats);
            Gs_SetState(0x6C, 1);
        }
        break;
    }
    }
}

/*
 * Allocates a gun effect with one block of the object buffer; returns NULL
 * when none is free. unusedType is not used.
 */
StageObj *GunFx_Alloc(u8 unusedType)
{
    return StageObj_Alloc(1);
}

/*
 * Creates the two effects of a gunlance shot on owner (a player) when owner
 * is in the current area: type 0, the flash, sized 0.8, 1.2, 1.2 or 2.0 by
 * shellLevel, and type 1, the smoke, with shellLevel in var2 (it picks the
 * part count). shellLevel is 0 to 2 from the gunlance's shell type, 3 for the
 * wyvern fire. It stops when the pool is full.
 */
void GunFx_CreateShell(void *owner, u8 shellLevel)
{
    StageObj *obj;
    u8 i;

    if (Obj_InCurrentArea(owner)) {
        for (i = 0; i < 2; i++) {
            obj = GunFx_Alloc(shellLevel);
            if (obj == NULL) {
                break;
            }
            obj->kind = STAGEOBJ_KIND_GUN_FX;
            obj->update = GunFx_Update;
            obj->owner = owner;
            obj->type = i;
            switch (i) {
            case 0:
                switch (shellLevel) {
                case 0:
                    obj->scale = 0.8f;
                    break;
                case 1:
                    obj->scale = 1.2f;
                    break;
                case 2:
                    obj->scale = 1.2f;
                    break;
                case 3:
                    obj->scale = 2.0f;
                    break;
                }
                break;
            case 1:
                obj->var2 = shellLevel;
                break;
            }
        }
    }
}

/*
 * Creates the spent casings of a reload on owner when owner is in the current
 * area: casingType 2 one bowgun casing, 3 the gunlance's three casings; any
 * other value creates nothing.
 */
void GunFx_CreateReload(void *owner, u8 casingType)
{
    StageObj *obj;

    if (Obj_InCurrentArea(owner)) {
        switch (casingType) {
        case 2:
            obj = StageObj_Alloc(1);
            if (obj != NULL) {
                obj->kind = STAGEOBJ_KIND_GUN_FX;
                obj->update = GunFx_Update;
                obj->owner = owner;
                obj->type = casingType;
            }
            break;
        case 3:
            obj = StageObj_Alloc(1);
            if (obj != NULL) {
                obj->kind = STAGEOBJ_KIND_GUN_FX;
                obj->update = GunFx_Update;
                obj->owner = owner;
                obj->type = casingType;
            }
            break;
        }
    }
}

/*
 * Creates the wyvern fire's charge effect (GUNFX_WYVERN_CHARGE) on owner, a
 * player in the current area.
 */
void GunFx_CreateType4(void *owner)
{
    StageObj *obj;

    if (Obj_InCurrentArea(owner)) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_GUN_FX;
            obj->update = GunFx_Update;
            obj->owner = owner;
            obj->type = 4;
        }
    }
}

/*
 * Creates the lasting effect GUNFX_HERMITAUR on owner, a Daimyo Hermitaur or
 * Shogun Ceanataur, with no area test.
 */
void GunFx_CreateType5(void *owner)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_GUN_FX;
        obj->update = GunFx_Update;
        obj->owner = owner;
        obj->type = 5;
    }
}

/*
 * Creates the wyvern fire's after-blast puffs (GUNFX_WYVERN_PUFFS) on owner,
 * a player in the current area.
 */
void GunFx_CreateType6(void *owner)
{
    StageObj *obj;

    if (Obj_InCurrentArea(owner)) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_GUN_FX;
            obj->update = GunFx_Update;
            obj->owner = owner;
            obj->type = 6;
        }
    }
}

/* Runs a versus bait's state: 0 Setup, 1 Run, 2 End, 3 Free. */
void VersusBait_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        VersusBait_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        VersusBait_Run(obj);
        break;
    case STAGEOBJ_END:
        VersusBait_End(obj);
        break;
    case STAGEOBJ_FREE:
        VersusBait_Free(obj);
        break;
    }
}

/*
 * Sets a versus bait's throw up and moves it to Run; frees the object when no
 * draw slot is free. It starts at the thrower's hand (VersusBait_HandOffset
 * in his frame) and aims at o->pos taken in the monster's frame: the landing
 * height goes to o->scale and the velocity to o->pos, the horizontal part
 * covering the distance in 21.5 frames. angle2 (its yaw) starts at the
 * throw's heading plus a quarter turn; angle, the yaw it settles at, is the
 * monster's heading plus up to 25 degrees either way, turned half a turn by a
 * test against angle2 and VersusBait_SettleYaw.
 */
void VersusBait_Setup(StageObj *obj)
{
    Player *thrower;
    u16 restYaw;
    u32 relYawSum;
    u16 relYaw;
    u16 throwYaw;
    f32 toTargetX;
    f32 toTargetY;
    f32 toTargetZ;
    f32 start[3];
    f32 target[3];

    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        thrower = &gPlayers[obj->ownerIndex];
        Vec_Rotate(start, VersusBait_HandOffset, (f32 *) thrower->rotMtx);
        Vec_Add(start, start, thrower->pos);
        Vec_Rotate(target, obj->pos, ((Em *) obj->owner)->rotMtx[0]);
        Vec_Add(target, target, &((Em *) obj->owner)->posX);
        obj->node->pos[0] = start[0];
        obj->node->pos[1] = start[1];
        obj->node->pos[2] = start[2];
        toTargetX = target[0] - start[0];
        toTargetY = target[1] - start[1];
        toTargetZ = target[2] - start[2];
        obj->angle2 = (s32) (0.5f + (65536.0f * Math_Atan2(toTargetX, toTargetZ)) / 6.2831855f);
        obj->angle2 += 0x4000;
        obj->var2 = 0xD5;
        restYaw = ((Em *) obj->owner)->rotY;
        restYaw += (u16) (s32) (0.5f + (65536.0f * (Rand_Next(1) % 50 - 25)) / 360.0f);
        relYaw = restYaw - D_008D8BA2;
        throwYaw = obj->angle2 + 0x1C72;
        relYaw -= throwYaw;
        relYawSum = relYaw + D_008D8BA2;
        if (relYawSum > 0xFFFF) {
            restYaw -= 0x8000;
        } else if (relYawSum > 0x8000) {
            restYaw -= 0x8000;
        }
        obj->angle = restYaw;
        obj->scale = target[1];
        obj->pos[0] = toTargetX / 21.5f;
        obj->pos[2] = toTargetZ / 21.5f;
        obj->pos[1] = 20.0f + 0.06666667f * toTargetY;
        obj->node->draw = VersusBait_Draw;
        return;
    }
    StageObj_Free(obj);
}

/*
 * Moves a versus bait for 0x65 frames, then hides it and moves to End. In
 * flight (var0 0) it falls (VersusBait_Move), spins its yaw and tumble (var2)
 * for 14 frames, eases them toward VersusBait_SettleYaw and
 * VersusBait_SettleAngles[0] by frame 24 and to angle and upright by frame
 * 30, makes Effect0D type 6 40 below it at frame 28, and bounces twice on the
 * landing height (Effect0D type 3, half its horizontal speed each time). From
 * frame 31 it rests on the landing height (var0 1). It queues its draw slot
 * each frame.
 */
void VersusBait_Run(StageObj *obj)
{
    s16 frame;
    u8 tumble;
    f32 dustPos[3];
    f32 *height;

    if (++obj->timer > 0x65) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    switch (obj->var0) {
    case 0:
        VersusBait_Move(obj);
        frame = obj->timer;
        if (frame <= 14) {
            obj->angle2 += 0x1E5;
            obj->var2 += 3;
        } else if (frame <= 24) {
            obj->angle2 += (u16) ((u16) ((u16) ((u16) obj->angle - D_008D8BA2) - obj->angle2) / (u16) (0x19 - (u16) frame));
            tumble = obj->var2;
            obj->var2 = tumble + (u8) ((((u16) (-(VersusBait_SettleAngles[0] + tumble) << 8) / (u16) (0x19 - (u16) obj->timer)) & 0xFF00) >> 8);
        } else if (frame <= 30) {
            obj->angle2 += (u16) ((u16) ((u16) obj->angle - obj->angle2) / (u16) (0x1F - (u16) frame));
            obj->var2 += (u8) ((u8) -obj->var2 / (u8) (0x1F - obj->timer));
        }
        if (obj->timer == 0x1C) {
            Vec_Copy(dustPos, obj->node->pos);
            dustPos[1] += -40.0f;
            PartFx_SpawnAt(dustPos, 6, 1.0f);
        }
        if (obj->timer >= 0x1F) {
            obj->var0++;
            obj->node->pos[1] = obj->scale;
        } else if (*(height = &obj->node->pos[1]) <= obj->scale) {
            *height = obj->scale;
            obj->pos[0] *= 0.5f;
            obj->pos[2] *= 0.5f;
            switch (obj->var1) {
            case 0:
                obj->pos[1] = 13.75f;
                PartFx_SpawnAt(obj->node->pos, 3, 1.0f);
                obj->var1++;
                break;
            case 1:
                obj->pos[1] = 8.75f;
                PartFx_SpawnAt(obj->node->pos, 3, 1.0f);
                obj->var1++;
                break;
            }
        }
        break;
    case 1:
        break;
    }
    OT_AddByDepth(gDrawOt32, obj->node, 0x20, 0);
}

/* Frees a versus bait's draw slot and moves to Free. */
void VersusBait_End(StageObj *obj)
{
    obj->state++;
    DrawSlot_Free(obj->slot);
}

/* Returns the versus bait to the stage object pool. */
void VersusBait_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws a versus bait: mesh 58 of the common model set, 1.5 times its size,
 * at the slot's position, turned by its tumble (var2, about x and z) and its
 * yaw (angle2), with RS_COLOR2 set to white.
 */
void VersusBait_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    Model *commonSet = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *mesh;
    f32 mtx[16];
    f32 tumble;

    if (commonSet != NULL && commonSet->active != 0 && obj->visible != 0) {
        tex = commonSet->mats;
        mesh = &commonSet->matSets[58];
        Mtx_Scale(mtx, 1.5f, 1.5f, 1.5f);
        Mtx_SetTrans(mtx, node->pos[0], node->pos[1], node->pos[2]);
        tumble = (6.2831855f * (f32) (obj->var2 << 8)) / 65536.0f;
        Mtx_RotateXZY(mtx, tumble, (6.2831855f * obj->angle2) / 65536.0f, tumble);
        Gs_SetState(0x67, -1);
        Effect_DrawModelAdd(mesh, (f32 *)((s32) mtx), tex);
    }
}

/*
 * Creates the bait thrower throws to em at a versus match's end, when em is
 * in the current area. throwPoint is the point it lands on, in em's frame;
 * baitType goes to o->type (the callers pass 0); the fourth argument is not
 * used.
 */
void VersusBait_Create(Player *thrower, Em *em, u8 baitType, s32 unused, f32 *throwPoint)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(0);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_VERSUS_BAIT;
            obj->update = VersusBait_Update;
            obj->owner = em;
            obj->type = baitType;
            obj->ownerIndex = thrower->index;
            Vec_Copy(obj->pos, throwPoint);
        }
    }
}

/*
 * Moves a versus bait one frame: takes 2.5 from its vertical speed, then adds
 * its velocity (o->pos) to its draw slot's position.
 */
void VersusBait_Move(StageObj *obj)
{
    obj->pos[1] -= 2.5f;
    obj->node->pos[0] += obj->pos[0];
    obj->node->pos[1] += obj->pos[1];
    obj->node->pos[2] += obj->pos[2];
}

/* Runs a blast charge's state: 0 Setup, 1 Run, 2 End, 3 Free. */
void TeostraBlastCharge_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        TeostraBlastCharge_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        TeostraBlastCharge_Run(obj);
        break;
    case STAGEOBJ_END:
        TeostraBlastCharge_End(obj);
        break;
    case STAGEOBJ_FREE:
        TeostraBlastCharge_Free(obj);
        break;
    }
}

/*
 * Sets a blast charge up around its monster: takes a draw slot for each of
 * its ten parts, starts gusts 0 to 5 with TeostraBlastCharge_InitGust and puffs 6
 * to 9 with a random lifetime, an outward speed of 45 and a random spin, and
 * puts each at the monster's bone 1. o->slot holds the number of parts in use
 * (10) and o->angle the monster's heading. Then it runs the first frame.
 */
void TeostraBlastCharge_Setup(StageObj *obj)
{
    TeostraBlastChargeWork *work = (TeostraBlastChargeWork *) obj->buf;
    TeostraBlastChargePart *p;
    s16 i = 0;

    obj->state++;
    p = work->part;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->var0 = 0;
    obj->node = NULL;
    obj->slot = 10;
    obj->angle = ((Em *) obj->owner)->rotY;
    obj->angle2 = 0;
    for (; i < 10; i++, p++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            switch (i) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                TeostraBlastCharge_InitGust(p, i);
                break;
            case 6:
            case 7:
            case 8:
            case 9:
                p->t = 0;
                p->life = 10.0f + 5.0f * (u32) Rand_Signed();
                p->speed = 45.0f;
                p->rotX = 0;
                p->rotY = 0;
                p->rotZ = 0.5f + (65536.0f * (360.0f * Rand_Unit())) / 360.0f;
                p->spin = 0.5f + (65536.0f * ((30.0f * Rand_Signed()) / 30.0f)) / 360.0f;
                break;
            }
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = obj;
            p->node->part = i;
            p->node->draw = TeostraBlastCharge_Draw;
            Mtx_Copy((f32 *) work->m, (f32 *) (Em_GetBone(obj->owner, 1)));
            Mtx_GetTrans(p->node->pos, work->m);
        } else {
            p->node = NULL;
        }
    }
    TeostraBlastCharge_Run(obj);
}

/*
 * Starts gust i: a rise of 200 to 300 per 30 frames, a delay of 5 * i frames,
 * a random tilt of 70 to 120 degrees about x, a random y angle and a random
 * spin of up to a third of a degree a frame.
 */
void TeostraBlastCharge_InitGust(TeostraBlastChargePart *gust, u8 i)
{
    gust->speed = (f32) ((200.0f + (100.0f * Rand_Unit())) / 30.0f);
    gust->t = i * -5;
    gust->rotX = (s16) (s32) (0.5f + ((65536.0f * (70.0f + (50.0f * Rand_Unit()))) / 360.0f));
    gust->rotY = (s16) (s32) (0.5f + ((65536.0f * (360.0f * Rand_Unit())) / 360.0f));
    gust->rotZ = 0;
    gust->spin = (u16) (s32) (0.5f + ((65536.0f * ((10.0f * Rand_Signed()) / 30.0f)) / 360.0f));
}

/*
 * Runs a blast charge's frame. Once the monster is neither in
 * KUSH_MOT_BLAST_CHARGE nor before frame 40 of KUSH_MOT_BLAST, var0 is set
 * and the gusts stop restarting. After 30 frames it frees the puffs once
 * (angle2 marks it), and moves to End when var0 is set and every part has
 * finished. Each gust rises and spins, and restarts at bone 1 after 30
 * frames; each puff moves out along its direction
 * (TeostraBlastCharge_PuffDirs turned by o->angle), slowing by 1.5 a frame
 * and rising from its life on.
 */
void TeostraBlastCharge_Run(StageObj *obj)
{
    TeostraBlastChargeWork *work = (TeostraBlastChargeWork *) obj->buf;
    TeostraBlastChargePart *p = work->part;
    Em *em = obj->owner;
    u8 i;
    Vec3f step;

    if ((em->motionId[0] != 0x424 || EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && ((Em *) obj->owner)->motionId[0] != 0x423) {
        obj->var0 = 1;
    }
    if (obj->timer >= 30) {
        if (obj->var0 == 1 && TeostraBlastCharge_AllGustsDone(obj) == 1) {
            obj->state++;
        } else if (obj->angle2 == 0) {
            obj->angle2 = 1;
            TeostraBlastCharge_FreePuffs(obj);
        }
    } else {
        obj->timer++;
    }
    for (i = 0; i < obj->slot; i++, p++) {
        if (p->node == NULL) {
            continue;
        }
        if (p->t++ < 0) {
            continue;
        }
        if (p->t >= 30) {
            if (obj->var0 != 1) {
                TeostraBlastCharge_InitGust(p, i);
                Mtx_Copy((f32 *) work->m, (f32 *) (Em_GetBone(obj->owner, 1)));
                Mtx_GetTrans(p->node->pos, work->m);
                p->t = -5;
            }
            continue;
        }
        switch (i) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            p->rotY += p->spin;
            p->node->pos[1] += p->speed;
            OT_AddByDepth(gDrawOt32, p->node, 0x40, 0);
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            p->rotZ += p->spin;
            step = TeostraBlastCharge_PuffDirs[i - 6];
            step.x *= p->speed;
            step.z *= p->speed;
            if (p->t >= p->life) {
                step.y = 7.5f;
            } else {
                step.y = 0.0f;
            }
            Vec_RotateY(&step.x, (6.2831855f * obj->angle) / 65536.0f);
            p->speed -= 1.5f;
            p->node->pos[0] += step.x;
            p->node->pos[1] += step.y;
            p->node->pos[2] += step.z;
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            break;
        }
    }
}

/*
 * Returns 1 when each of the o->slot parts in use has reached frame 30, else
 * 0.
 */
s32 TeostraBlastCharge_AllGustsDone(StageObj *obj)
{
    s16 count = obj->slot;
    u8 *partBytes = obj->buf;
    u8 i;

    for (i = 0; i < count; i++) {
        if (*(s8 *) (partBytes + 2) < 30) {
            return 0;
        }
        partBytes += 0x14;
    }
    return 1;
}

/* Frees the draw slots of the ten parts and moves to Free. */
void TeostraBlastCharge_End(StageObj *obj)
{
    u8 *partBytes = obj->buf;
    s16 i;

    for (i = 0; i < 10; i++) {
        if (*(s32 *) (partBytes + 4) != 0) {
            DrawSlot_Free(*(s16 *) partBytes);
        }
        partBytes += 0x14;
    }
    obj->state++;
}

/*
 * Frees the draw slots of the four puffs (parts 6 to 9), clears their nodes
 * and sets o->slot to 6, so only the gusts are counted from then on.
 */
void TeostraBlastCharge_FreePuffs(StageObj *obj)
{
    u8 *partBytes = obj->buf + 0x78;
    s16 i;

    for (i = 0; i < 4; i++) {
        if (*(s32 *) (partBytes + 4) != 0) {
            *(s32 *) (partBytes + 4) = 0;
            DrawSlot_Free(*(s16 *) partBytes);
        }
        partBytes += 0x14;
    }
    obj->slot = 6;
}

/* Returns the blast charge to the stage object pool. */
void TeostraBlastCharge_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Returns the monster slot of the first kind of
 * TeostraBlastCharge_MonsterKinds that is loaded (Lunastra's, then
 * Teostra's), or -1 when none is.
 */
s16 TeostraBlastCharge_FindMonsterSlot(void)
{
    u8 *kindPtr = TeostraBlastCharge_MonsterKinds;
    u8 kind = *kindPtr;
    s16 slot;

    while (kind != 0xFF) {
        slot = EmSlot_FindKind(kind);
        if (slot != -1) {
            return slot;
        }
        kindPtr++;
        kind = *kindPtr;
    }
    return -1;
}

/*
 * Draws one part of a blast charge with the part models (gScene.partModels)
 * of the Lunastra or Teostra slot; draws nothing when there is none. Gusts 0
 * to 5 use mesh 7 with blend bit 2 and the gust key curves, puffs 6 to 9 mesh
 * 1 with blend bit 4 and the puff key curves; each is scaled and coloured by
 * its frame and turned by its three angles, and parts from 3 on face the
 * camera.
 */
void TeostraBlastCharge_Draw(DrawSlot *node)
{
    s32 color;
    StageObj *obj = node->owner;
    f32 scale[3];
    TeostraBlastChargePart *p = (TeostraBlastChargePart *) obj->buf + node->part;
    ModelMatSet *model;
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    u8 *scaleKeys;
    u16 mode = 0;
    ModelMat *mats;
    u8 *colorKeys;
    f32 uvScroll;
    Model *set;
    s16 monsterSlot;

    monsterSlot = TeostraBlastCharge_FindMonsterSlot();
    if (monsterSlot != -1 && (set = gScene.partModels[monsterSlot]) != NULL && set->active && obj->visible) {
        switch (node->part) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            mode |= 2;
            model = &set->matSets[7];
            scaleKeys = TeostraBlastCharge_GustScaleKeys;
            colorKeys = TeostraBlastCharge_GustColorKeys;
            uvScroll = p->t / 30;
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            model = &set->matSets[1];
            mode |= 4;
            scaleKeys = TeostraBlastCharge_PuffScaleKeys;
            colorKeys = TeostraBlastCharge_PuffColorKeys;
            uvScroll = 0.0f;
            break;
        }
        mats = set->mats;
        Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, p->t);
        rot[0] = (6.2831855f * p->rotX) / 65536.0f;
        rot[1] = (6.2831855f * p->rotY) / 65536.0f;
        rot[2] = (6.2831855f * p->rotZ) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, mtx);
        if ((u32) node->part >= 3) {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        Color_EvalKeys((s32 *) colorKeys, p->t, (u32 *) &color);
        Mtx_Translation(uvMtx, 0.0f, uvScroll, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, mtx, color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

/* Creates a blast charge on em when em is in the current area. */
void TeostraBlastCharge_Create(Em *em)
{
    StageObj *obj;

    if (Player_InCurrentArea(em)) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_TEOSTRA_BLAST_CHARGE;
            obj->update = TeostraBlastCharge_Update;
            obj->owner = em;
            obj->type = 0;
        }
    }
}

/* Runs a player effect's state: 0 Setup, 1 Run, 2 End, 3 Free. */
void PlayerFx_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        PlayerFx_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        PlayerFx_Run(obj);
        break;
    case STAGEOBJ_END:
        PlayerFx_End(obj);
        break;
    case STAGEOBJ_FREE:
        PlayerFx_Free(obj);
        break;
    }
}

/*
 * Sets a player effect up by type: takes its start point or bone matrix from
 * the owner (bone 10, bone 0x12, the horn's note bone, or a point ahead of
 * the player), plays its start sound, then starts each of its
 * PlayerFx_PartCounts[type] parts (part type, delay, angles, spin and offset
 * from the type's tables or at random) with a draw slot each, from the second
 * pool when the object is kept. var2 is 0 with an owner and 0xFF without.
 * Type 6 maps var1 1 to colour row 0 and 2 to row 1 at 1.8 times the size,
 * and frees the object for any other var1.
 */
void PlayerFx_Setup(StageObj *obj)
{
    PlayerFxPart *p = (PlayerFxPart *) obj->buf;
    Player *owner = obj->owner;
    s16 count;
    s16 i;
    s16 bitIndex;
    u16 bits;
    f32 boneMtx[16];
    f32 offset[3];

    obj->state++;
    obj->var0 = 0;
    if (obj->owner != NULL) {
        obj->var2 = 0;
    } else {
        obj->var2 = 0xFF;
    }
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    count = PlayerFx_PartCounts[obj->type];
    switch (obj->type) {
    case 0:
    case 1:
    case 4:
        Em_GetBonePos(owner, 10, obj->pos);
        break;
    case 2:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(owner, PL_BONE_OTHER_HAND)));
        break;
    case 3:
        bits = Rand_Next(1);
        break;
    case 6:
        Em_GetBonePos(owner, 10, obj->pos);
        obj->pos[1] = owner->floorY + 30.0f * obj->scale;
        bits = Rand_Next(1);
        switch (obj->var1) {
        case 1:
            obj->var1 = 0;
            break;
        case 2:
            obj->var1 = 1;
            obj->scale *= 1.8f;
            break;
        default:
            PlayerFx_Free(obj);
            return;
        }
        break;
    case 8:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(owner, 10)));
        break;
    case 9:
        Em_GetBonePos(owner, 10, obj->pos);
        bits = Rand_Next(1);
        break;
    case 10:
        Vec_Copy(obj->pos, ((Player *) obj->owner)->pos);
        offset[0] = 25.0f;
        offset[1] = 0.0f;
        offset[2] = 120.0f;
        Vec_RotateY(offset, (6.2831855f * ((Player *) obj->owner)->rot[1]) / 65536.0f);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        obj->angle2 = ((Player *) obj->owner)->rot[1];
        break;
    case 11:
        obj->var1 = HuntingHorn_PlayingNoteColour(obj->owner);
        Mtx_GetTrans(obj->pos, owner->weaponModel->partBones[0][3].world);
        break;
    case 14:
        bits = Rand_Next(1);
        bitIndex = 0;
        break;
    case 15:
        bits = Rand_Next(1);
        bitIndex = 0;
        break;
    }
    PlayerFx_PlayStartSound(obj);
    for (i = 0; i < count; i++, p++) {
        p->frame = 0;
        p->type = i;
        p->variant = 0;
        p->scale = obj->scale;
        p->useColorKeys = 1;
        p->scale2D = 1;
        p->rotX = 0;
        p->rotY = 0;
        p->rotZ = 0;
        p->spinX = 0;
        p->spinY = 0;
        p->spinZ = 0;
        p->speed = 0.0f;
        switch (obj->type) {
        case 0:
            Vec_Copy(p->pos, obj->pos);
            p->frame = -PlayerFx_SlashDelays[i];
            switch (i) {
            case 0:
            case 1:
            case 3:
                break;
            case 2:
                p->rotZ = Rand_Next(1);
                break;
            case 4:
                p->scale2D = 0;
                p->pos[1] = owner->floorY;
                break;
            default:
            case 5:
            case 6:
            case 7:
                p->type = 5;
                p->rotX = Rand_Next(1);
                p->rotY = Rand_Next(1);
                p->rotZ = Rand_Next(1);
                bits = Rand_Next(1);
                p->spinZ = ((bits & 0xFFF) + 0x1555) / (f32) PlayerFx_SlashDurations[p->type];
                if (bits & 0x1000) {
                    p->spinZ = -p->spinZ;
                }
                break;
            }
            break;
        case 4:
            Vec_Copy(p->pos, obj->pos);
            p->frame = -PlayerFx_SlashDelays[i];
            switch (i) {
            case 0:
            case 1:
                break;
            case 2:
                p->rotZ = Rand_Next(1);
                break;
            case 3:
                p->type = 4;
                p->scale2D = 0;
                p->pos[1] = owner->floorY;
                break;
            }
            break;
        case 1:
            p->frame = -D_008D9008[i];
            switch (i) {
            case 0:
            case 1:
                p->type = 0;
                break;
            case 2:
                p->type = 1;
                break;
            case 3:
            case 4:
                p->type = 2;
                p->speed = 0.0f;
                break;
            case 5:
                p->type = 3;
                break;
            case 6:
                p->type = 4;
                p->speed = 0.0f;
                break;
            }
            break;
        case 2:
            p->type = PlayerFx_BowChargePartTypes[i];
            PlayerFx_InitBowChargePart(obj, p, (EmBone *) boneMtx, i);
            break;
        case 3:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_BowShotPartTypes[i];
            p->frame = -PlayerFx_BowShotDelays[i];
            switch (p->type) {
            case 0:
                p->rotZ = Rand_Next(1);
                break;
            case 1:
                p->rotX = obj->angle2;
                p->rotY = obj->angle;
                break;
            case 2:
                p->rotX = (Rand_Next(1) & 0xFFF) - 0x800;
                p->rotY = obj->angle + (Rand_Next(1) & 0xFFF) - 0x800;
                p->rotZ = Rand_Next(1);
                p->spinZ = ((u16) (s32) (0.5f + (65536.0f * (40.0f * Rand_Unit())) / 360.0f) + 0x1C72) / (f32) PlayerFx_BowShotDurations[p->type];
                if (bits & (1 << i)) {
                    p->spinZ = -p->spinZ;
                }
                break;
            }
            break;
        case 5:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_TrapPartTypes[i];
            p->frame = -PlayerFx_TrapDelays[i];
            switch (p->type) {
            case 0:
                p->pos[1] += 20.0f;
                p->rotX = 0x4000;
                break;
            default:
            case 1:
                p->scale2D = 0;
                p->pos[1] += 10.0f;
                break;
            }
            break;
        case 6:
        case 9:
            Vec_Copy(p->pos, obj->pos);
            p->pos[0] += obj->scale * (50.0f * Rand_Signed());
            p->pos[2] += obj->scale * (50.0f * Rand_Signed());
            p->type = bits & 1;
            p->rotZ = bits;
            p->spinZ = 0x5B;
            if ((bits >> 1) & 1) {
                p->spinZ = -p->spinZ;
            }
            break;
        case 7:
            p->type = 0;
            p->frame = -PlayerFx_HornNoteDelays[i];
            p->scale = 0.8f + 0.7f * Rand_Unit();
            p->rotZ = Rand_Next(1);
            p->spinZ = 0.5f + (65536.0f * ((360.0f * Rand_Signed()) / PlayerFx_HornNoteDurations[p->type])) / 360.0f;
            break;
        case 8:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_GreatSwordChargePartTypes[i];
            PlayerFx_InitGreatSwordChargePart(obj, p, (EmBone *) boneMtx, i);
            switch (p->type) {
            case 2:
            case 3:
                p->frame = PlayerFx_GreatSwordChargeDurations[p->type];
                break;
            }
            break;
        case 10:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_ImpactPartTypes[i];
            p->frame = -PlayerFx_ImpactDelays[i];
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = PlayerFx_ImpactDepths[i % 3];
            Vec_RotateY(offset, (6.2831855f * obj->angle2) / 65536.0f);
            p->pos[0] += offset[0];
            p->pos[1] += offset[1];
            p->pos[2] += offset[2];
            switch (p->type) {
            case 0:
                p->rotZ = 0x4000;
                p->scale2D = 1;
                break;
            case 1:
                p->variant = PlayerFx_ImpactMeshes[Rand_Next(1) % 2];
                p->speed = (10.0f + 10.0f * Rand_Unit()) / PlayerFx_ImpactDurations[p->type];
                p->rotZ = Rand_Next(1);
                p->spinZ = 0.5f + (65536.0f * ((10.0f * Rand_Signed()) / (u8) PlayerFx_ImpactDurations[p->type])) / 360.0f;
                p->scale2D = 1;
                break;
            }
            break;
        case 11:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_HornPlayPartTypes[i];
            p->frame = -PlayerFx_HornPlayDelays[i];
            switch (p->type) {
            case 0:
                p->rotZ = Rand_Next(1);
                break;
            case 1:
                break;
            }
            break;
        case 12:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_DragonPartTypes[i];
            p->frame = -PlayerFx_DragonDelays[i];
            if (obj->var1 == 0) {
                p->frame -= 5;
            }
            p->rotZ = Rand_Next(1);
            switch (p->type) {
            case 0:
                break;
            case 1:
                p->speed = 2.0f;
                p->pos[0] += 50.0f * Rand_Signed();
                p->pos[1] += 50.0f * Rand_Signed();
                p->pos[2] += 50.0f * Rand_Signed();
                break;
            case 2:
                p->type += Rand_Next(1) & 1;
                p->speed = 0.2f;
                p->pos[0] += 30.0f * Rand_Signed();
                p->pos[1] += 30.0f * Rand_Signed();
                p->pos[2] += 30.0f * Rand_Signed();
                break;
            }
            break;
        case 13:
            Vec_Copy(p->pos, obj->pos);
            p->frame = -PlayerFx_BombFlameDelays[i];
            switch (p->type) {
            case 1:
                p->variant = Rand_Next(1) & 3;
                break;
            }
            break;
        case 14:
            Vec_Copy(p->pos, obj->pos);
            p->type = PlayerFx_BombRingSmokePartTypes[i];
            p->frame = -D_008D9E58[i];
            switch (p->type) {
            case 0:
                if (i == 1) {
                    p->rotX = 0x4000;
                    p->variant = 1;
                }
                break;
            case 1:
                break;
            case 2:
                p->scale2D = 0;
                break;
            case 3:
                p->type += (bits >> bitIndex++) & 1;
                p->rotZ = Rand_Next(1);
                p->pos[0] += obj->scale * (50.0f * Rand_Signed());
                p->pos[1] += obj->scale * (80.0f + 50.0f * Rand_Signed());
                p->pos[2] += obj->scale * (50.0f * Rand_Signed());
                p->speed = obj->scale * ((100.0f + 50.0f * Rand_Unit()) / 30.0f);
                break;
            }
            break;
        case 15:
            Vec_Copy(p->pos, obj->pos);
            p->frame = i * -4;
            p->pos[0] += obj->scale * (15.0f * Rand_Signed());
            p->pos[2] += obj->scale * (15.0f * Rand_Signed());
            p->type = (bits >> bitIndex++) & 1;
            p->rotZ = Rand_Next(1);
            p->spinZ = 0x111;
            if ((bits >> bitIndex++) & 1) {
                p->spinZ = -p->spinZ;
            }
            break;
        case 16:
            p->type = 0;
            p->scale *= 1.5f;
            Vec_Copy(p->pos, obj->pos);
            p->rotZ = Rand_Next(1);
            break;
        }
        if (obj->keep == 0) {
            p->slot = DrawSlot_Alloc();
        } else {
            p->slot = DrawSlotB_Alloc();
        }
        if (p->slot != -1) {
            if (obj->keep == 0) {
                p->node = DrawSlot_Get(p->slot);
            } else {
                p->node = DrawSlotB_Get(p->slot);
            }
            p->node->owner = obj;
            p->node->part = i;
            p->node->draw = PlayerFx_Draw;
        } else {
            p->node = NULL;
        }
    }
}

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

extern s16 PlayerFx_Lifetimes[];
extern s16 *PlayerFx_Durations[];
extern s16 PlayerFx_FirstCurve[];
extern s16 PlayerFx_FirstColorKeys[];
extern void *PlayerFx_Curves[];
extern u8 *PlayerFx_ColorKeys[];
extern u8 PlayerFx_BowChargeFullPartTypes[];
extern u8 PlayerFx_BowChargeAlphas[];
extern f32 PlayerFx_HitJitter[];
extern s16 D_008D8FF8[];
s32 PlayerFx_OwnerGone(void *obj);
u8 Bow_ChargeLevel(Player *p);
u8 GreatSword_ChargeLevel(Player *p);
u8 HuntingHorn_PlayingNote(Player *p);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Vec_RotateX(f32 *v, f32 angle);
void PlayerFx_RestartHornNotePart(PlayerFxPart *w);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);

/*
 * Runs a player effect's frame; it ends after PlayerFx_Lifetimes[type] frames
 * (1000: never) or for types 2, 7 and 8 when their owner is gone. Types 2 and
 * 8 follow the bow's or the great sword's charge level in var2 (a sound at
 * each new level, parts restarted) and end when the owner leaves the charge
 * motions; type 7 follows the horn only while a note plays; types 12 and 15
 * play their sounds, and 15 ends after var1 frames. Each part counts its
 * frame up to its last frame (the type's Durations), restarting by type, and
 * while shown takes its scale and colour from the type's key curves, moves by
 * type, spins and queues its draw slot.
 */
void PlayerFx_Run(StageObj *obj)
{
    PlayerFxPart *part = (PlayerFxPart *) obj->buf;
    Player *owner = obj->owner;
    s16 life;
    s16 count;
    s16 i;
    s16 lastFrame;
    s32 partTime;
    s16 curveTime;
    s16 curveIdx;
    s32 colorIdx;
    s16 tableOfs;
    void *curve;
    s16 *lastFrames;
    u8 prevLevel;
    u16 bits;
    f32 boneMtx[16];
    f32 jitter[3];
    f32 offset[3];
    f32 alpha;
    u16 dirPitch;
    u16 dirYaw;

    life = PlayerFx_Lifetimes[obj->type];
    obj->timer++;
    if (life != 1000 && obj->timer > life) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    count = PlayerFx_PartCounts[obj->type];
    lastFrames = PlayerFx_Durations[obj->type];
    if (obj->var2 != 0xFF && PlayerFx_OwnerGone(owner) == 1) {
        obj->var2 = 0xFF;
    }
    switch (obj->type) {
    case 2:
        if (obj->var2 != 0xFF) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(owner, PL_BONE_OTHER_HAND)));
            Vec_DirToAngles(boneMtx, (s16 *) &dirPitch, (s16 *) &dirYaw);
            prevLevel = obj->var2;
            obj->var2 = Bow_ChargeLevel(owner);
            if (((Player *) obj->owner)->motionId[0] != 0x57D && ((Player *) obj->owner)->motionId[0] != 0x3F1 && ((Player *) obj->owner)->motionId[0] != 0x3F4 && ((Player *) obj->owner)->motionId[0] != 0x3F5 && ((Player *) obj->owner)->motionId[0] != 0x58A && ((Player *) obj->owner)->motionId[0] != 0x58B && ((Player *) obj->owner)->motionId[0] != 0x58C) {
                obj->var2 = 0xFF;
            }
        }
        if (obj->var2 == 0xFF) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (Bow_ArcVisible(owner, 1) == 1) {
            obj->visible = 1;
        } else {
            obj->visible = 0;
        }
        if (prevLevel != obj->var2) {
            switch (obj->var2) {
            case 1:
                Player_PlaySe(obj->owner, 0xC, 0, &boneMtx[12], 1, 0);
                break;
            case 2:
                Player_PlaySe(obj->owner, 0xD, 0, &boneMtx[12], 1, 0);
                break;
            case 3:
                Player_PlaySe(obj->owner, 0xE, 0, &boneMtx[12], 1, 0);
                Player_PlaySe(obj->owner, 0x12, 0, &boneMtx[12], 1, 0);
                break;
            }
        }
        switch (obj->var2) {
        case 3:
            if (obj->var1 < 0xFF) {
                obj->var1++;
            }
            break;
        }
        break;
    case 7:
        if (obj->var2 == 0xFF) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (!(Obj_InCurrentArea(owner) & 0xFF) || !(HuntingHorn_PlayingNote(owner) & 0xFF)) {
            return;
        }
        Mtx_GetTrans(obj->pos, owner->weaponModel->partBones[0][3].world);
        break;
    case 8:
        if (obj->var2 != 0xFF) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(owner, 10)));
            prevLevel = obj->var2;
            obj->var2 = GreatSword_ChargeLevel(owner);
            if (((Player *) obj->owner)->motionId[0] != 0x579 && ((Player *) obj->owner)->motionId[0] != 0x58C && ((Player *) obj->owner)->motionId[0] != 0x58A && ((Player *) obj->owner)->motionId[0] != 0x590 && ((Player *) obj->owner)->motionId[0] != 0x592 && ((Player *) obj->owner)->motionId[0] != 0x595 && ((Player *) obj->owner)->motionId[0] != 0x596) {
                obj->var2 = 0xFF;
            }
            Mtx_GetTrans(obj->pos, boneMtx);
        }
        if (obj->var2 == 0xFF) {
            obj->state++;
            obj->visible = 0;
            Player_PlaySe(obj->owner, 0x1E, 0, ((Player *) obj->owner)->pos, 1, 0);
            Player_PlaySe(obj->owner, 0x1F, 0, ((Player *) obj->owner)->pos, 1, 0);
            return;
        }
        if (obj->timer == 0x2D) {
            Player_PlaySe(obj->owner, 0x1D, 0, &boneMtx[12], 1, 0);
        }
        break;
    case 12:
        if (obj->var1 == 0 && obj->timer == 5) {
            Sound_PlaySeAtPosChecked(1, 0xB7, 0, obj->pos, 1, 0);
        }
        break;
    case 15:
        if (obj->timer > obj->var1) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        if (obj->timer % 14 == 1) {
            Sound_PlaySeAtPosChecked(7, 10, 0, obj->pos, 0x11, 0);
        }
        break;
    }
    for (i = 0; i < count; i++, part++) {
        lastFrame = lastFrames[part->type];
        switch (obj->type) {
        case 2:
            if (prevLevel != obj->var2) {
                switch (obj->var2) {
                case 1:
                case 2:
                    switch (part->type) {
                    case 3:
                    case 4:
                        part->frame = 0;
                        PlayerFx_InitBowChargePart(obj, part, (EmBone *) boneMtx, i);
                        break;
                    }
                    break;
                case 3:
                    switch (PlayerFx_BowChargeFullPartTypes[i]) {
                    case 5:
                    case 7:
                        break;
                    default:
                        part->type = PlayerFx_BowChargeFullPartTypes[i];
                        PlayerFx_InitBowChargePart(obj, part, (EmBone *) boneMtx, i);
                        break;
                    }
                    break;
                }
            }
            if (obj->var2 == 3 && obj->var1 == 5) {
                switch (PlayerFx_BowChargeFullPartTypes[i]) {
                case 5:
                case 7:
                    part->type = PlayerFx_BowChargeFullPartTypes[i];
                    PlayerFx_InitBowChargePart(obj, part, (EmBone *) boneMtx, i);
                    break;
                }
            }
            break;
        case 7:
            if (part->type == 0xFF) {
                continue;
            }
            break;
        case 8:
            if (prevLevel != obj->var2) {
                switch (obj->var2) {
                case 1:
                case 2:
                    switch (part->type) {
                    case 2:
                    case 3:
                        part->frame = 0;
                        PlayerFx_InitGreatSwordChargePart(obj, part, (EmBone *) boneMtx, i);
                        break;
                    }
                    break;
                }
            }
            break;
        case 10:
            switch (part->type) {
            case 0:
            if (obj->timer > lastFrame + PlayerFx_ImpactDelays[i]) {
                Vec_Copy(part->pos, obj->pos);
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = PlayerFx_ImpactDepths[i % 3];
                Vec_RotateY(offset, (6.2831855f * obj->angle2) / 65536.0f);
                part->pos[0] += offset[0];
                part->pos[1] += offset[1];
                part->pos[2] += offset[2];
                part->type = PlayerFx_ImpactPartTypes[i + 6];
                part->frame = -PlayerFx_ImpactDelays[i + 6];
                part->scale2D = 0;
                part->speed = 15.0f;
                part->rotZ = Rand_Next(1);
            }
            break;
            }
            break;
        }
        if (++part->frame > lastFrame) {
            switch (obj->type) {
            case 1:
                switch (part->type) {
                case 0:
                    part->type = 2;
                    switch (i) {
                    case 0:
                        part->frame = -9;
                        break;
                    case 1:
                        part->frame = -13;
                        break;
                    }
                    break;
                case 2:
                    if (obj->timer < 0x29) {
                        part->frame = 1;
                    }
                    break;
                case 4:
                    if (obj->timer < 0x2E) {
                        part->frame = 1;
                    }
                    break;
                }
                break;
            case 2:
                switch (part->type) {
                case 0:
                    part->frame = 0x1F;
                    break;
                case 1:
                    part->frame = 1;
                    part->rotZ = Rand_Next(1);
                    break;
                case 2:
                case 5:
                case 6:
                    part->frame = 1;
                    break;
                }
                break;
            case 7:
                PlayerFx_RestartHornNotePart(part);
                break;
            case 8:
                switch (part->type) {
                case 0:
                case 1:
                    part->frame = PlayerFx_GreatSwordChargeDurations[part->type];
                    break;
                default:
                    part->frame = PlayerFx_GreatSwordChargeDurations[part->type] + 1;
                    break;
                }
                break;
            case 15:
                if (obj->timer < obj->var1 - 8) {
                    Vec_Copy(part->pos, obj->pos);
                    part->pos[0] += obj->scale * (15.0f * Rand_Signed());
                    part->pos[2] += obj->scale * (15.0f * Rand_Signed());
                    part->frame = -2;
                    bits = Rand_Next(1);
                    part->type = bits & 1;
                    part->rotZ = bits;
                    if ((bits >> 1) & 1) {
                        part->spinZ = -part->spinZ;
                    }
                } else {
                    part->frame = lastFrame;
                }
                break;
            }
        }
        if (part->frame > 0 && part->frame <= lastFrame) {
            switch (obj->type) {
            case 1:
                if (part->frame == 1) {
                    part->rotZ = Rand_Next(1);
                    Em_GetBonePos(obj->owner, 10, part->pos);
                    switch (part->type) {
                    case 2:
                        bits = Rand_Next(1);
                        part->spinZ = 2731.0f / D_008D8FF8[part->type];
                        if (bits & 1) {
                            part->spinZ = -part->spinZ;
                        }
                        Vec_Copy(jitter, PlayerFx_HitJitter);
                        jitter[0] *= Rand_Signed();
                        jitter[1] *= Rand_Signed();
                        jitter[2] *= Rand_Signed();
                        Vec_Add(part->pos, part->pos, jitter);
                        break;
                    case 3:
                        Vec_Copy(jitter, PlayerFx_HitJitter);
                        jitter[0] *= Rand_Signed();
                        jitter[1] *= Rand_Signed();
                        jitter[2] *= Rand_Signed();
                        Vec_Add(part->pos, part->pos, jitter);
                        break;
                    }
                }
                break;
            case 8:
                if (part->frame == 1) {
                    switch (part->type) {
                    case 2:
                        Player_PlaySe(obj->owner, 0xD, 0, &boneMtx[12], 1, 0);
                        break;
                    }
                }
                break;
            }
            curveIdx = PlayerFx_FirstCurve[obj->type];
            colorIdx = PlayerFx_FirstColorKeys[obj->type];
            partTime = part->frame;
            curveTime = partTime;
            switch (obj->type) {
            case 6:
            case 9:
                colorIdx = (s16) (colorIdx + obj->var1);
                break;
            case 8:
                switch (part->type) {
                case 0:
                    curveTime = (partTime & 3) + 1;
                    break;
                }
                tableOfs = part->type;
                curveIdx += tableOfs;
                colorIdx = (s16) (colorIdx + tableOfs);
                break;
            case 11:
                switch (part->type) {
                case 0:
                    colorIdx = (s16) (obj->var1 - 1 + colorIdx);
                    break;
                case 1:
                    colorIdx = (s16) (obj->var1 - 1 + (colorIdx + 6));
                    break;
                }
                break;
            case 16:
                tableOfs = part->type + obj->var1;
                curveIdx += tableOfs;
                colorIdx = (s16) (colorIdx + tableOfs);
                break;
            default:
                tableOfs = part->type;
                curveIdx += tableOfs;
                colorIdx = (s16) (colorIdx + tableOfs);
                break;
            }
            if (part->scale2D) {
                curve = PlayerFx_Curves[curveIdx];
                curveIdx++;
                Key_LerpVec2(curve, part->sc, curveTime);
            } else {
                curve = PlayerFx_Curves[curveIdx];
                curveIdx++;
                Key_LerpVec3(curve, part->sc, curveTime);
            }
            if (part->useColorKeys == 0) {
                Key_LerpFloat(PlayerFx_Curves[curveIdx], &alpha, (s16) partTime);
                part->color = ((u8) (u32) (255.0f * alpha) << 24) | 0xFFFFFF;
            } else {
                Color_EvalKeys((s32 *) (PlayerFx_ColorKeys[(s16) colorIdx]), (s16) partTime, (u32 *) &part->color);
            }
            switch (obj->type) {
            case 0:
            case 4:
                switch (part->type) {
                case 0:
                case 1:
                case 2:
                case 3:
                    Em_GetBonePos(obj->owner, 10, part->pos);
                    break;
                }
                break;
            case 1:
                switch (part->type) {
                case 2:
                    part->speed += -0.3f;
                    part->pos[1] += part->speed;
                    break;
                case 4:
                    part->speed += -0.5f;
                    part->pos[1] += part->speed;
                    break;
                default:
                    Em_GetBonePos(obj->owner, 10, part->pos);
                    break;
                }
                break;
            case 2:
                switch (part->type) {
                default:
                    offset[0] = 0.0f;
                    break;
                case 1:
                    offset[0] = -part->speed * part->frame;
                    break;
                case 2:
                    offset[0] = -150.0f;
                    part->color = (part->color & 0xFF) | ((((part->color >> 8) & 0xFF) << 8) | ((PlayerFx_BowChargeAlphas[obj->var2] << 24) | (((part->color >> 16) & 0xFF) << 16)));
                    part->rotX = dirPitch;
                    part->rotY = dirYaw;
                    break;
                case 3:
                    offset[0] = -110.0f;
                    break;
                case 4:
                    offset[0] = -150.0f;
                    part->rotX = dirPitch;
                    part->rotY = dirYaw;
                    break;
                case 5:
                    offset[0] = -150.0f;
                    break;
                case 6:
                    offset[0] = -175.0f;
                    part->rotX = dirPitch;
                    part->rotY = dirYaw;
                    break;
                case 7:
                    offset[0] = -110.0f;
                    break;
                case 8:
                    offset[0] = -150.0f;
                    part->rotX = dirPitch;
                    part->rotY = dirYaw;
                    break;
                case 9:
                    offset[0] = -150.0f;
                    break;
                }
                offset[1] = 0.0f;
                offset[2] = 0.0f;
                Vec_RotateSelf((f32 *) offset, (f32 *) boneMtx);
                Mtx_GetTrans(part->pos, boneMtx);
                part->pos[0] += offset[0];
                part->pos[1] += offset[1];
                part->pos[2] += offset[2];
                break;
            case 3:
                switch (part->type) {
                case 2:
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 150.0f / lastFrame;
                    Vec_RotateX(offset, (6.2831855f * obj->angle2) / 65536.0f);
                    Vec_RotateY(offset, (6.2831855f * obj->angle) / 65536.0f);
                    part->pos[0] += offset[0];
                    part->pos[1] += offset[1];
                    part->pos[2] += offset[2];
                    break;
                }
                break;
            case 6:
            case 9:
                part->pos[1] += 3.0f * obj->scale;
                break;
            case 7:
            case 8:
                Vec_Copy(part->pos, obj->pos);
                break;
            case 10:
                switch (part->type) {
                case 0:
                    break;
                case 1:
                    part->pos[1] += part->speed;
                    break;
                case 2:
                    part->pos[1] += part->speed;
                    part->speed -= 3.0f;
                    break;
                }
                break;
            case 11:
                Mtx_GetTrans(obj->pos, owner->weaponModel->partBones[0][3].world);
                break;
            case 12:
                switch (part->type) {
                case 1:
                    part->speed -= 2.5f;
                    part->pos[1] += part->speed;
                    break;
                case 2:
                case 3:
                    part->speed -= 0.5f;
                    part->pos[1] += part->speed;
                    break;
                }
                break;
            case 14:
                switch (part->type) {
                case 3:
                case 4:
                    part->pos[1] += part->speed;
                    break;
                }
                break;
            case 15:
                part->pos[1] += 100.0f * obj->scale;
                break;
            case 16:
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 50.0f;
                Vec_RotateSelf((f32 *) offset, gCamMatrix);
                part->pos[0] = obj->pos[0] + offset[0];
                part->pos[1] = obj->pos[1] + offset[1];
                part->pos[2] = obj->pos[2] + offset[2];
                break;
            }
            part->rotX += part->spinX;
            part->rotY += part->spinY;
            part->rotZ += part->spinZ;
            if (part->node != NULL) {
                Vec_Copy(part->node->pos, part->pos);
                OT_AddByDepth(gDrawOtSorted, part->node, 0x40, 0);
            }
        }
    }
}

/*
 * Frees the draw slots of the effect's parts (to the second pool when the
 * object is kept) and moves to Free.
 */
void PlayerFx_End(StageObj *obj)
{
    u8 *partBytes = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = PlayerFx_PartCounts[obj->type];
    for (i = 0; i < count; i++) {
        if (*(s32 *) (partBytes + 0x24) != 0) {
            if (obj->keep == 0) {
                DrawSlot_Free(*(s16 *) partBytes);
            } else {
                DrawSlotB_Free(*(s16 *) partBytes);
            }
        }
        partBytes += 0x3C;
    }
}

/* Returns the player effect to the stage object pool. */
void PlayerFx_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws one part of a player effect with a mesh of the common model set: the
 * type's mesh table entry for the part type (type 10's part type 1 uses
 * part->variant as the mesh). Each type sets the blend bits, the rotation
 * order and the facing: 0 the part's own angles, 1 facing the camera, 2
 * turned with the camera's yaw, 3 a camera billboard turned a quarter turn
 * about x, its scale applied after. Type 10 takes the area's effect colour
 * (Effect_GetColor) with the part's alpha scaled by the area's. partType
 * keeps part->type for the switches of types 1, 13 and 14; the other types
 * read part->type again after their calls, as the game's code does.
 */
void PlayerFx_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    u8 facing;
    PlayerFxPart *p = (PlayerFxPart *) obj->buf + node->part;
    u16 rotOrder = 0;
    u16 mode = 0;
    ModelMatSet *model;
    ModelMat *mats;
    u16 *meshIds;
    u32 alpha;
    s32 facingMode;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    f32 *scaleY;
    f32 *scaleZ;
    f32 axisScale;
    u8 red;
    u8 green;
    u8 blue;
    u8 areaAlpha;
    u32 objType;
    u8 partType;
    Model *set;

    facing = 1;

    if (obj->visible && (objType = obj->type, meshIds = PlayerFx_MeshTbl[objType], (set = gStageModelSet) != NULL) && set->active) {
        if (objType == 10 && (partType = p->type) == 1) {
            model = &set->matSets[p->variant];
        } else {
            partType = p->type;
            model = &set->matSets[meshIds[partType]];
        }
        mats = set->mats;
        switch (objType) {
        case 0:
        case 4:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            switch (p->type) {
            default:
            case 0:
            case 1:
            case 2:
                mode |= 2;
                rotOrder = 3;
                break;
            case 3:
                rotOrder = 3;
                break;
            case 4:
                rotOrder = 0;
                facing = 0;
                break;
            case 5:
                mode |= 2;
                rotOrder = 7;
                facing = 0;
                break;
            }
            break;
        case 1:
            switch (partType) {
            case 1:
                Mtx_Translation(uvMtx, 0.75f, 0.25f, 0.0f);
                break;
            default:
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                break;
            }
            Gs_SetState(0x19, (s32) uvMtx);
            rotOrder = 3;
            facing = 1;
            switch (p->type) {
            case 0:
            case 1:
                mode |= 2;
                break;
            case 3:
                mode |= 2;
                break;
            case 2:
                mode |= 4;
                break;
            case 4:
                mode |= 4;
                break;
            }
            break;
        case 2:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 2;
            switch (p->type) {
            case 0:
            case 5:
                rotOrder = 5;
                break;
            case 1:
            case 3:
            case 7:
            case 9:
                rotOrder = 3;
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                rotOrder = 4;
                facing = 3;
                break;
            default:
                return;
            }
            break;
        case 3:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 2;
            switch (p->type) {
            case 0:
                rotOrder = 3;
                break;
            case 1:
                rotOrder = 4;
                facing = 3;
                break;
            case 2:
                rotOrder = 7;
                facing = 0;
                break;
            default:
                return;
            }
            break;
        case 5:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 2;
            facing = 0;
            switch (p->type) {
            case 0:
                rotOrder = 1;
                break;
            case 1:
                break;
            default:
                return;
            }
            break;
        case 6:
        case 9:
        case 15:
            rotOrder = 3;
            break;
        case 7:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 4;
            rotOrder = 3;
            facing = 1;
            break;
        case 8:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 2;
            switch (p->type) {
            case 0:
            case 1:
                rotOrder = 5;
                break;
            case 2:
            case 3:
                rotOrder = 3;
                break;
            default:
                return;
            }
            break;
        case 10:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 4;
            Effect_GetColor(gScene.area, &red, &green, &blue, &areaAlpha);
            alpha = (u32) (((f32) (u32) areaAlpha / 255.0f) * (f32) ((p->color >> 24) & 0xFF));
            p->color = blue | ((green << 8) | ((red << 16) | ((alpha & 0xFF) << 24)));
            switch (p->type) {
            case 0:
                facing = 2;
                rotOrder = 3;
                break;
            case 1:
                facing = 1;
                rotOrder = 3;
                break;
            case 2:
                facing = 0;
                rotOrder = 3;
                break;
            default:
                return;
            }
            break;
        case 11:
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            mode |= 2;
            facing = 1;
            rotOrder = 3;
            break;
        case 12:
            facing = 1;
            rotOrder = 3;
            break;
        case 13:
            mode |= 2;
            facing = 2;
            switch (partType) {
            case 0:
                break;
            case 1:
                Mtx_Translation(uvMtx, 0.25f * (p->variant & 1), 0.0625f * ((p->variant >> 1) & 1), 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            }
            break;
        case 14:
            switch (partType) {
            case 0:
                if (p->variant == 0) {
                    facing = 1;
                } else {
                    rotOrder = 1;
                    facing = 0;
                }
                mode |= 2;
                break;
            case 1:
                facing = 1;
                mode |= 2;
                break;
            case 2:
                facing = 0;
                mode |= 2;
                break;
            case 3:
            case 4:
                facing = 1;
                rotOrder = 3;
                break;
            default:
                return;
            }
            break;
        case 16:
            facing = 1;
            if (obj->var1 == 0) {
                mode |= 2;
            } else {
                mode |= 8;
            }
            rotOrder = 3;
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        default:
            return;
        }
        facingMode = facing;
        if (facingMode == 3) {
            scaleY = &scale[1];
            scaleZ = &scale[2];
            scale[0] = 1.0f;
            *scaleY = 1.0f;
            *scaleZ = 1.0f;
        } else {
            scaleY = &scale[1];
            scaleZ = &scale[2];
            scale[0] = p->sc[0] * p->scale;
            *scaleY = p->sc[1] * p->scale;
            *scaleZ = p->sc[2] * p->scale;
        }
        rot[0] = (6.2831855f * (f32) (u32) p->rotX) / 65536.0f;
        rot[1] = (6.2831855f * (f32) (u32) p->rotY) / 65536.0f;
        rot[2] = (6.2831855f * (f32) (u32) p->rotZ) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, rotOrder, mtx);
        switch (facingMode) {
        case 0:
            break;
        case 1:
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 2:
            Mtx_Concat3x3(mtx, gCamYawMtx);
            break;
        case 3:
            Mtx_Billboard(mtx, node->pos);
            Mtx_TurnX(mtx, 1.5707964f);
            scale[0] = p->sc[0] * p->scale;
            *scaleY = p->sc[1] * p->scale;
            *scaleZ = p->sc[2] * p->scale;
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
            break;
        }
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(model, mtx, p->color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

/* Plays the start sound of types 0, 4 and 8 at the owner's position. */
void PlayerFx_PlayStartSound(StageObj *obj)
{
    switch (obj->type) {
    case 0:
        Player_PlaySe(obj->owner, 0xC, 0, ((Player *) obj->owner)->pos, 1, 0);
        break;
    case 4:
        Player_PlaySe(obj->owner, 0xE, 0, ((Player *) obj->owner)->pos, 1, 0);
        break;
    case 8:
        Player_PlaySe(obj->owner, 0x1C, 0, ((Player *) obj->owner)->pos, 1, 0);
        break;
    }
}

/* Returns 1 when owner is inactive or its ObjHead.unk004 is 3, else 0. */
s32 PlayerFx_OwnerGone(void *owner)
{
    ObjHead *head = owner;

    if (head->active == 0 || head->unk004 == 3) {
        return 1;
    }
    return 0;
}

/*
 * Starts part of a bow charge at boneMtx's position by its part type: 0 and 5
 * a random z angle and a slow spin, every odd part turned half a turn; 1 a
 * random z angle and a drift of 70 over its frames; 3 and 4 sized by the
 * charge level (PlayerFx_BowChargeScales[var2]), 4 after 2 frames; 7 three
 * times the size; 8 1.5 times after 2 frames; 9 a random z angle after 4
 * frames. i is the part's index.
 */
void PlayerFx_InitBowChargePart(StageObj *obj, PlayerFxPart *part, EmBone *boneMtx, s16 i)
{
    Mtx_GetTrans(part->pos, boneMtx->world);
    part->useColorKeys = 1;
    part->scale = obj->scale;
    part->scale2D = 1;
    switch (part->type) {
    case 0:
    case 5:
        part->rotZ = Rand_Next(1);
        part->spinZ = 0xB6;
        if (i & 1) {
            part->rotY = 0x8000;
        }
        break;
    case 1:
        part->rotZ = Rand_Next(1);
        part->speed = 70.0f / PlayerFx_BowChargeDurations[part->type];
        break;
    case 3:
        part->scale *= PlayerFx_BowChargeScales[obj->var2];
        break;
    case 4:
        part->frame = -2;
        part->scale *= PlayerFx_BowChargeScales[obj->var2];
        break;
    case 7:
        part->scale *= 3.0f;
        break;
    case 8:
        part->frame = -2;
        part->scale *= 1.5f;
        break;
    case 9:
        part->frame = -4;
        part->rotZ = Rand_Next(1);
        break;
    }
}

/* Zero padding after the last jump table, to the end of the overlay's read-only data: two
 * objects, because the compiler aligns an object of 0x18 bytes to 16 bytes. */
const u8 D_008F51E8[8] = {0};
const u8 D_008F51F0[0x10] = {0};

/*
 * Restarts part of a horn note effect at frame 1 with a random size of 0.8 to
 * 1.5, a random z angle and a random spin over its frames.
 */
void PlayerFx_RestartHornNotePart(PlayerFxPart *part)
{
    part->frame = 1;
    part->scale = 0.8f + (0.7f * Rand_Unit());
    part->rotZ = Rand_Next(1);
    part->spinZ = (s16) (s32) (0.5f + ((65536.0f * ((360.0f * Rand_Signed()) / (f32) PlayerFx_HornNoteDurations[part->type])) / 360.0f));
}

/*
 * Starts part of a great sword charge at boneMtx's position by its part type:
 * 0 and 1 a random z angle and a spin, every odd part turned half a turn; 2
 * and 3 sized by the charge level (PlayerFx_GreatSwordChargeScales[var2]), 2
 * with a random z angle.
 */
void PlayerFx_InitGreatSwordChargePart(StageObj *obj, PlayerFxPart *part, EmBone *boneMtx, s16 i)
{
    Mtx_GetTrans(part->pos, boneMtx->world);
    part->useColorKeys = 1;
    part->scale = obj->scale;
    part->scale2D = 1;
    switch (part->type) {
    case 0:
        part->rotZ = Rand_Next(1);
        part->spinZ = 0x16C;
        if (i & 1) {
            part->rotY = 0x8000;
        }
        break;
    case 1:
        part->rotZ = Rand_Next(1);
        part->spinZ = 0x7F;
        if (i & 1) {
            part->rotY = 0x8000;
        }
        break;
    case 2:
        part->scale *= PlayerFx_GreatSwordChargeScales[obj->var2];
        part->rotZ = Rand_Next(1);
        part->spinZ = 0;
        break;
    case 3:
        part->scale *= PlayerFx_GreatSwordChargeScales[obj->var2];
        part->rotZ = 0;
        part->spinZ = 0;
        break;
    }
}

/*
 * Allocates a player effect with one block of the object buffer; returns NULL
 * when none is free. unusedType is not used.
 */
StageObj *PlayerFx_Alloc(u8 unusedType)
{
    return StageObj_Alloc(1);
}

/* Creates a player effect of fxType on player with variant 0. */
void PlayerFx_CreateOnPlayerDefault(Player *player, u8 fxType, f32 scale)
{
    PlayerFx_CreateOnPlayer(player, fxType, 0, scale);
}

/*
 * Creates a player effect of fxType on player, with variant in var1 and scale
 * as its size. Types 2, 3, 8 and 10 are made for the local player only; type
 * 7 is kept across the stage's object frees and made in any area, the others
 * only when player is in the current area. Type 3 starts 110 along the axis
 * of bone 0x12 and takes that axis's pitch and yaw as angle2 and angle.
 */
void PlayerFx_CreateOnPlayer(Player *player, u8 fxType, s32 variant, f32 scale)
{
    u8 kept;
    s32 typeNum;
    StageObj *obj;
    EmBone *bone;
    f32 offset[3];

    kept = 0;
    typeNum = fxType;
    if (typeNum == 2 || typeNum == 3 || typeNum == 8 || typeNum == 10) {
        if (!Player_IsLocal(player)) {
            return;
        }
    }
    if (typeNum == 7) {
        kept = 1;
    }
    if (kept == 0 && !(Obj_InCurrentArea(player) & 0xFF)) {
        return;
    }
    if ((obj = PlayerFx_Alloc(fxType)) == NULL) {
        return;
    }
    obj->kind = STAGEOBJ_KIND_PLAYER_FX;
    obj->update = PlayerFx_Update;
    obj->owner = player;
    obj->type = fxType;
    obj->scale = scale;
    obj->var1 = variant;
    if (kept != 0) {
        obj->keep = 1;
    }
    if (obj->type == 3) {
        bone = Em_GetBone(player, PL_BONE_OTHER_HAND);
        Vec_DirToAngles((f32 *) bone, (s16 *) &obj->angle2, (s16 *) &obj->angle);
        Mtx_GetTrans(obj->pos, bone->world);
        offset[0] = -110.0f;
        offset[1] = 0.0f;
        offset[2] = 0.0f;
        Vec_RotateSelf((f32 *) offset, (f32 *) bone);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
    }
}

/* Creates a player effect of fxType at pos, with no owner and variant 0. */
void PlayerFx_CreateAtDefault(f32 *pos, u8 fxType, f32 scale)
{
    PlayerFx_CreateAt(pos, fxType, 0, scale);
}

/*
 * Creates a player effect of fxType at pos, with no owner, variant in var1
 * and scale as its size.
 */
void PlayerFx_CreateAt(f32 *src, u8 fxType, u8 variant, f32 scale)
{
    StageObj *obj = PlayerFx_Alloc(fxType);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_PLAYER_FX;
        obj->update = PlayerFx_Update;
        obj->owner = NULL;
        obj->type = fxType;
        obj->scale = scale;
        obj->var1 = variant;
        Vec_Copy(obj->pos, src);
    }
}

/*
 * Creates a floor hazard effect (half size) on player every 16 frames while
 * hazard is nonzero: 1 a poison floor (variant row 0), 2 a damaging floor or
 * a monster's aura (row 1). Player_UpdateEnvironment calls it each frame.
 */
void PlayerFx_TickFloorHazard(Player *player, s32 hazard)
{
    if ((u8) hazard && !(gScene.frame & 0xF)) {
        PlayerFx_CreateOnPlayer(player, 6, hazard, 0.5f);
    }
}

/*
 * Creates the status 6 effect (variant 0) on player on every 16th frame;
 * Player_Update calls it while PL_STATUS_ICON_STATUS6 shows.
 */
void PlayerFx_TickStatus6(Player *player)
{
    if (!(gScene.frame & 0xF)) {
        PlayerFx_CreateOnPlayer(player, 9, 0, 1.0f);
    }
}

/*
 * Creates the status 7 effect (variant 1) on player on every 16th frame, 8
 * frames after the status 6 one; Player_Update calls it while
 * PL_STATUS_ICON_STATUS7 shows.
 */
void PlayerFx_TickStatus7(Player *player)
{
    if ((gScene.frame & 0xF) == 8) {
        PlayerFx_CreateOnPlayer(player, 9, 1, 1.0f);
    }
}

/* Runs a bow arc's state: 0 Setup, 1 Run, 2 End, 3 Free. */
void BowArc_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        BowArc_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        BowArc_Run(obj);
        break;
    case STAGEOBJ_END:
        BowArc_End(obj);
        break;
    case STAGEOBJ_FREE:
        BowArc_Free(obj);
        break;
    }
}

/*
 * Sets a bow arc up: colour phase and timer at 0 and a draw slot from the
 * second pool for BowArc_Draw; then runs its first frame.
 */
void BowArc_Setup(StageObj *obj)
{
    BowArcWork *work = (BowArcWork *) obj->buf;

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    work->timer = 0;
    work->phase = 0;
    obj->slot = DrawSlotB_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlotB_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = BowArc_Draw;
    }
    BowArc_Run(obj);
}

/*
 * While Bow_ArcVisible(archer, 0) holds: flips the arc's colour phase every
 * 24 frames, takes the position and the pitch and yaw of the archer's bone
 * 0xE (its x axis), and queues its draw slot.
 */
void BowArc_Run(StageObj *obj)
{
    BowArcWork *work;
    Em *archer = obj->owner;
    EmBone *bone;
    f32 dir[3];
    u16 dirPitch;
    u16 dirYaw;

    if (Bow_ArcVisible(archer, 0) != 0) {
        work = (BowArcWork *) obj->buf;
        work->timer++;
        if (work->timer >= 0x18) {
            work->phase++;
            work->timer = 0;
            if (work->phase >= 2) {
                work->phase = 0;
            }
        }
        dir[0] = 1.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
        bone = Em_GetBone(archer, 0xE);
        Vec_RotateSelf((f32 *) dir, (f32 *) bone);
        Mtx_GetTrans(obj->pos, bone->world);
        Vec_DirToAngles(dir, (s16 *) &dirPitch, (s16 *) &dirYaw);
        work->yaw = dirPitch;
        work->pitch = dirYaw;
        work->unk08 = 0;
        obj->node->pos[0] = obj->pos[0];
        obj->node->pos[1] = obj->pos[1];
        obj->node->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
    }
}

/* Frees the bow arc's draw slot (second pool) and moves to Free. */
void BowArc_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlotB_Free(obj->slot);
    }
}

/* Returns the bow arc to the stage object pool. */
void BowArc_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws a bow's arc for its owner when he is the local player in the current
 * area, with a bow equipped, active and visible. Bones 2 to 17 of the common
 * models' set 2 are placed at every second step of the arrow's flight from
 * bone 0xE along its pitch and yaw (BowArrow_FlightParams: the speed, then
 * gravity after a count of steps), and the set is drawn with a colour pulsing
 * with the frame: red and green in phase 0, red alone in phase 1.
 */
void BowArc_Draw(DrawSlot *node)
{
    s16 straightSteps;
    f32 gravity;
    f32 pos[3];
    f32 vel[3];
    f32 mtx[16];
    f32 scaleMtx[16];
    StageObj *obj = node->owner;
    Player *pl = obj->owner;
    BowArcWork *work = (BowArcWork *) obj->buf;
    s32 i;
    s32 boneIdx;
    Model *set;
    ModelMatSet *model;
    u8 *bones;
    ModelMat *tex;
    u8 red;
    u8 green;

    if (pl->weaponType != WEAPON_BOW || pl->active == 0 || pl->visible == 0) {
        return;
    }
    if (gScene.area == pl->area && Player_IsLocal(pl)) {
        set = gStageModelSets[2];
        if (set != NULL && set->active != 0) {
            Gs_SetState(0x60, 0);
            Gfx_SetTexFilter(1);
            tex = set->mats;
            model = set->matSets;
            Mtx_Scale(scaleMtx, 1.75f, 1.75f, 1.0f);
            BowArrow_FlightParams(0, (s32 *) vel, &gravity, &straightSteps);
            Mtx_Identity(mtx);
            Mtx_RotateX(mtx, (6.2831855f * work->yaw) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * work->pitch) / 65536.0f);
            Vec_RotateSelf((f32 *) vel, (f32 *) mtx);
            Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
            Mtx_Concat3x3(mtx, scaleMtx);
            Bone_UpdateWorld(set->bones, mtx);
            bones = (u8 *)set->bones;
            Vec_Copy(pos, obj->pos);
            boneIdx = 2;
            for (i = 0; i < 32; i++) {
                if (!(i & 1)) {
                    Mtx_SetTrans((f32 *) (bones + boneIdx * 0x190), pos[0], pos[1], pos[2]);
                    boneIdx++;
                    if (boneIdx >= 18) {
                        break;
                    }
                }
                pos[0] += vel[0];
                pos[1] += vel[1];
                pos[2] += vel[2];
                if (straightSteps <= 0) {
                    vel[1] -= gravity;
                } else {
                    straightSteps--;
                }
            }
            red = 0x40;
            green = red;
            switch (work->phase) {
            case 0:
                red = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xA0;
                green = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xA0;
                break;
            case 1:
                red = (u8) (63.0f * Math_Sin((6.2831855f * ((gScene.frame & 0xF) << 12)) / 65536.0f)) + 0xB0;
                break;
            }
            Bone_StoreMatrices(set->bones);
            Gs_SetState(0x67, (red << 16) | 0xC0000000 | (green << 8) | 0x40);
            for (i = 0; i < set->matSetCount; i++) {
                if (model != NULL) {
                    if (model->id != -1) {
                        Gs_SetLightList(tex, model);
                        Draw_ApplyStates(model->flags);
                        Model_Draw(model->id, 0);
                    }
                    model++;
                }
            }
            Draw_SetDefaultState();
        }
    }
}

/*
 * Creates the bow arc that follows owner (a player with a bow), kept across
 * the stage's object frees.
 */
void BowArc_Create(void *owner)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_BOW_ARC;
        obj->update = BowArc_Update;
        obj->owner = owner;
        obj->keep = 1;
    }
}

/* The file's data. */
extern u32 D_008F5CE8[];
extern u32 __start[];
f32 VersusBait_HandOffset[4] = {
#include "game/008D8B90.inc"
};
u16 VersusBait_SettleAngles[4] = {
#include "game/008D8BA0.inc"
};
u8 TeostraBlastCharge_MonsterKinds[8] = {
#include "game/008D8BA8.inc"
};
u8 TeostraBlastCharge_GustScaleKeys[64] = {
#include "game/008D8BB0.inc"
};
u8 TeostraBlastCharge_GustColorKeys[32] = {
#include "game/008D8BF0.inc"
};
u8 TeostraBlastCharge_PuffScaleKeys[64] = {
#include "game/008D8C10.inc"
};
u8 TeostraBlastCharge_PuffColorKeys[48] = {
#include "game/008D8C50.inc"
};
Vec3f TeostraBlastCharge_PuffDirs[4] = {
#include "game/008D8C80.inc"
};
u32 PlayerFx_Curves_0[8] = {
#include "game/008D8CB0.inc"
};
u32 PlayerFx_Curves_1[8] = {
#include "game/008D8CD0.inc"
};
u32 PlayerFx_Curves_2[12] = {
#include "game/008D8CF0.inc"
};
u32 PlayerFx_Curves_3[8] = {
#include "game/008D8D20.inc"
};
u32 PlayerFx_Curves_4[12] = {
#include "game/008D8D40.inc"
};
u32 PlayerFx_Curves_5[8] = {
#include "game/008D8D70.inc"
};
u8 PlayerFx_ColorKeys_0[32] = {
#include "game/008D8D90.inc"
};
u8 PlayerFx_ColorKeys_1[32] = {
#include "game/008D8DB0.inc"
};
u8 PlayerFx_ColorKeys_2[32] = {
#include "game/008D8DD0.inc"
};
u8 PlayerFx_ColorKeys_3[32] = {
#include "game/008D8DF0.inc"
};
u8 PlayerFx_ColorKeys_4[32] = {
#include "game/008D8E10.inc"
};
u8 PlayerFx_ColorKeys_5[24] = {
#include "game/008D8E30.inc"
};
u16 PlayerFx_DrawTbl_0[6] = {
#include "game/008D8E48.inc"
};
s16 PlayerFx_SlashDurations[6] = {
#include "game/008D8E58.inc"
};
s16 PlayerFx_SlashDelays[8] = {
#include "game/008D8E70.inc"
};
u32 PlayerFx_Curves_6[12] = {
#include "game/008D8E80.inc"
};
u32 PlayerFx_Curves_7[8] = {
#include "game/008D8EB0.inc"
};
u32 PlayerFx_Curves_8[8] = {
#include "game/008D8ED0.inc"
};
u32 PlayerFx_Curves_9[8] = {
#include "game/008D8EF0.inc"
};
u32 PlayerFx_Curves_10[12] = {
#include "game/008D8F10.inc"
};
u8 PlayerFx_ColorKeys_6[32] = {
#include "game/008D8F40.inc"
};
u8 PlayerFx_ColorKeys_7[32] = {
#include "game/008D8F60.inc"
};
u8 PlayerFx_ColorKeys_8[48] = {
#include "game/008D8F80.inc"
};
u8 PlayerFx_ColorKeys_9[32] = {
#include "game/008D8FB0.inc"
};
u8 PlayerFx_ColorKeys_10[72] = {
#include "game/008D8FD0.inc"
};
f32 PlayerFx_HitJitter[3] = {
#include "game/008D9018.inc"
};
u32 PlayerFx_Curves_11[16] = {
#include "game/008D9030.inc"
};
u32 PlayerFx_Curves_12[8] = {
#include "game/008D9070.inc"
};
u32 PlayerFx_Curves_13[12] = {
#include "game/008D9090.inc"
};
u32 PlayerFx_Curves_14[8] = {
#include "game/008D90C0.inc"
};
u32 PlayerFx_Curves_15[8] = {
#include "game/008D90E0.inc"
};
u32 PlayerFx_Curves_16[12] = {
#include "game/008D9100.inc"
};
u32 PlayerFx_Curves_17[12] = {
#include "game/008D9130.inc"
};
u32 PlayerFx_Curves_18[8] = {
#include "game/008D9160.inc"
};
u32 PlayerFx_Curves_19[8] = {
#include "game/008D9180.inc"
};
u32 PlayerFx_Curves_20[8] = {
#include "game/008D91A0.inc"
};
u8 PlayerFx_ColorKeys_11[32] = {
#include "game/008D91C0.inc"
};
u8 PlayerFx_ColorKeys_12[32] = {
#include "game/008D91E0.inc"
};
u8 PlayerFx_ColorKeys_13[32] = {
#include "game/008D9200.inc"
};
u8 PlayerFx_ColorKeys_14[32] = {
#include "game/008D9220.inc"
};
u8 PlayerFx_ColorKeys_15[32] = {
#include "game/008D9240.inc"
};
u8 PlayerFx_ColorKeys_16[32] = {
#include "game/008D9260.inc"
};
u8 PlayerFx_ColorKeys_17[32] = {
#include "game/008D9280.inc"
};
u8 PlayerFx_ColorKeys_18[32] = {
#include "game/008D92A0.inc"
};
u8 PlayerFx_ColorKeys_19[32] = {
#include "game/008D92C0.inc"
};
u8 PlayerFx_ColorKeys_20[24] = {
#include "game/008D92E0.inc"
};
u8 PlayerFx_BowChargePartTypes[8] = {
#include "game/008D92F8.inc"
};
u8 PlayerFx_BowChargeFullPartTypes[16] = {
#include "game/008D9300.inc"
};
u16 PlayerFx_DrawTbl_2[16] = {
#include "game/008D9310.inc"
};
s16 PlayerFx_BowChargeDurations[12] = {
#include "game/008D9330.inc"
};
u8 PlayerFx_BowChargeAlphas[8] = {
#include "game/008D9348.inc"
};
f32 PlayerFx_BowChargeScales[4] = {
#include "game/008D9350.inc"
};
u32 PlayerFx_Curves_21[8] = {
#include "game/008D9360.inc"
};
u32 PlayerFx_Curves_22[8] = {
#include "game/008D9380.inc"
};
u32 PlayerFx_Curves_23[12] = {
#include "game/008D93A0.inc"
};
u8 PlayerFx_ColorKeys_21[32] = {
#include "game/008D93D0.inc"
};
u8 PlayerFx_ColorKeys_22[32] = {
#include "game/008D93F0.inc"
};
u8 PlayerFx_ColorKeys_23[24] = {
#include "game/008D9410.inc"
};
u8 PlayerFx_BowShotPartTypes[8] = {
#include "game/008D9428.inc"
};
u16 PlayerFx_BowShotDelays[8] = {
#include "game/008D9430.inc"
};
u16 PlayerFx_DrawTbl_3[4] = {
#include "game/008D9440.inc"
};
s16 PlayerFx_BowShotDurations[4] = {
#include "game/008D9448.inc"
};
u32 PlayerFx_Curves_24[12] = {
#include "game/008D9450.inc"
};
u32 PlayerFx_Curves_25[16] = {
#include "game/008D9480.inc"
};
u8 PlayerFx_ColorKeys_24[32] = {
#include "game/008D94C0.inc"
};
u8 PlayerFx_ColorKeys_25[32] = {
#include "game/008D94E0.inc"
};
u8 PlayerFx_TrapPartTypes[8] = {
#include "game/008D9500.inc"
};
u16 PlayerFx_TrapDelays[4] = {
#include "game/008D9508.inc"
};
u16 PlayerFx_DrawTbl_5[4] = {
#include "game/008D9510.inc"
};
s16 PlayerFx_Durations_5[4] = {
#include "game/008D9518.inc"
};
u32 PlayerFx_Curves_26[8] = {
#include "game/008D9520.inc"
};
u8 PlayerFx_ColorKeys_26[48] = {
#include "game/008D9540.inc"
};
u8 PlayerFx_ColorKeys_27[40] = {
#include "game/008D9570.inc"
};
u16 PlayerFx_DrawTbl_6[4] = {
#include "game/008D9598.inc"
};
s16 PlayerFx_Durations_6[8] = {
#include "game/008D95A0.inc"
};
u32 PlayerFx_Curves_27[12] = {
#include "game/008D95B0.inc"
};
u8 PlayerFx_ColorKeys_28[32] = {
#include "game/008D95E0.inc"
};
s16 PlayerFx_HornNoteDelays[4] = {
#include "game/008D9600.inc"
};
s16 PlayerFx_HornNoteDurations[4] = {
#include "game/008D9608.inc"
};
u16 PlayerFx_DrawTbl_7[8] = {
#include "game/008D9610.inc"
};
u32 PlayerFx_Curves_28[12] = {
#include "game/008D9620.inc"
};
u32 PlayerFx_Curves_29[8] = {
#include "game/008D9650.inc"
};
u32 PlayerFx_Curves_30[12] = {
#include "game/008D9670.inc"
};
u32 PlayerFx_Curves_31[12] = {
#include "game/008D96A0.inc"
};
u8 PlayerFx_ColorKeys_29[32] = {
#include "game/008D96D0.inc"
};
u8 PlayerFx_ColorKeys_30[32] = {
#include "game/008D96F0.inc"
};
u8 PlayerFx_ColorKeys_31[32] = {
#include "game/008D9710.inc"
};
u8 PlayerFx_ColorKeys_32[32] = {
#include "game/008D9730.inc"
};
u8 PlayerFx_GreatSwordChargePartTypes[8] = {
#include "game/008D9750.inc"
};
u16 PlayerFx_DrawTbl_8[4] = {
#include "game/008D9758.inc"
};
s16 PlayerFx_GreatSwordChargeDurations[4] = {
#include "game/008D9760.inc"
};
f32 PlayerFx_GreatSwordChargeScales[3] = {
#include "game/008D9768.inc"
};
u8 PlayerFx_ColorKeys_33[48] = {
#include "game/008D9780.inc"
};
u8 PlayerFx_ColorKeys_34[40] = {
#include "game/008D97B0.inc"
};
u16 PlayerFx_DrawTbl_10[4] = {
#include "game/008D97D8.inc"
};
u8 PlayerFx_ImpactMeshes[16] = {
#include "game/008D97E0.inc"
};
u32 PlayerFx_Curves_32[12] = {
#include "game/008D97F0.inc"
};
u32 PlayerFx_Curves_33[12] = {
#include "game/008D9820.inc"
};
u32 PlayerFx_Curves_34[16] = {
#include "game/008D9850.inc"
};
u8 PlayerFx_ColorKeys_35[32] = {
#include "game/008D9890.inc"
};
u8 PlayerFx_ColorKeys_36[32] = {
#include "game/008D98B0.inc"
};
u8 PlayerFx_ColorKeys_37[24] = {
#include "game/008D98D0.inc"
};
f32 PlayerFx_ImpactDepths[3] = {
#include "game/008D98E8.inc"
};
u8 PlayerFx_ImpactPartTypes[12] = {
#include "game/008D98F8.inc"
};
u16 PlayerFx_ImpactDelays[12] = {
#include "game/008D9910.inc"
};
s16 PlayerFx_ImpactDurations[4] = {
#include "game/008D9928.inc"
};
u16 PlayerFx_DrawTbl_11[8] = {
#include "game/008D9930.inc"
};
u32 PlayerFx_Curves_35[12] = {
#include "game/008D9940.inc"
};
u32 PlayerFx_Curves_36[12] = {
#include "game/008D9970.inc"
};
u8 PlayerFx_ColorKeys_38[32] = {
#include "game/008D99A0.inc"
};
u8 PlayerFx_ColorKeys_39[32] = {
#include "game/008D99C0.inc"
};
u8 PlayerFx_ColorKeys_40[32] = {
#include "game/008D99E0.inc"
};
u8 PlayerFx_ColorKeys_41[32] = {
#include "game/008D9A00.inc"
};
u8 PlayerFx_ColorKeys_42[32] = {
#include "game/008D9A20.inc"
};
u8 PlayerFx_ColorKeys_43[32] = {
#include "game/008D9A40.inc"
};
u8 PlayerFx_ColorKeys_44[32] = {
#include "game/008D9A60.inc"
};
u8 PlayerFx_ColorKeys_45[32] = {
#include "game/008D9A80.inc"
};
u8 PlayerFx_ColorKeys_46[32] = {
#include "game/008D9AA0.inc"
};
u8 PlayerFx_ColorKeys_47[32] = {
#include "game/008D9AC0.inc"
};
u8 PlayerFx_ColorKeys_48[32] = {
#include "game/008D9AE0.inc"
};
u8 PlayerFx_ColorKeys_49[32] = {
#include "game/008D9B00.inc"
};
u8 PlayerFx_HornPlayPartTypes[8] = {
#include "game/008D9B20.inc"
};
s16 PlayerFx_Durations_11[4] = {
#include "game/008D9B28.inc"
};
s16 PlayerFx_HornPlayDelays[4] = {
#include "game/008D9B30.inc"
};
u16 PlayerFx_DrawTbl_12[4] = {
#include "game/008D9B38.inc"
};
u32 PlayerFx_Curves_37[8] = {
#include "game/008D9B40.inc"
};
u32 PlayerFx_Curves_38[12] = {
#include "game/008D9B60.inc"
};
u32 PlayerFx_Curves_39[12] = {
#include "game/008D9B90.inc"
};
u8 PlayerFx_ColorKeys_50[32] = {
#include "game/008D9BC0.inc"
};
u8 PlayerFx_ColorKeys_51[32] = {
#include "game/008D9BE0.inc"
};
u8 PlayerFx_ColorKeys_52[32] = {
#include "game/008D9C00.inc"
};
u8 PlayerFx_DragonPartTypes[8] = {
#include "game/008D9C20.inc"
};
s16 PlayerFx_DragonDelays[4] = {
#include "game/008D9C28.inc"
};
s16 PlayerFx_Durations_12[4] = {
#include "game/008D9C30.inc"
};
u16 PlayerFx_DrawTbl_13[4] = {
#include "game/008D9C38.inc"
};
u32 PlayerFx_Curves_41[16] = {
#include "game/008D9C40.inc"
};
u32 PlayerFx_Curves_42[12] = {
#include "game/008D9C80.inc"
};
u8 PlayerFx_ColorKeys_54[32] = {
#include "game/008D9CB0.inc"
};
u8 PlayerFx_ColorKeys_55[40] = {
#include "game/008D9CD0.inc"
};
s16 PlayerFx_BombFlameDelays[4] = {
#include "game/008D9CF8.inc"
};
u32 PlayerFx_Durations_13[2] = {
    (u32)__start, 0,
};
u16 PlayerFx_DrawTbl_14[6] = {
#include "game/008D9D08.inc"
};
u32 PlayerFx_Curves_43[12] = {
#include "game/008D9D20.inc"
};
u32 PlayerFx_Curves_44[8] = {
#include "game/008D9D50.inc"
};
u32 PlayerFx_Curves_45[12] = {
#include "game/008D9D70.inc"
};
u32 PlayerFx_Curves_46[8] = {
#include "game/008D9DA0.inc"
};
u8 PlayerFx_ColorKeys_56[48] = {
#include "game/008D9DC0.inc"
};
u8 PlayerFx_ColorKeys_57[32] = {
#include "game/008D9DF0.inc"
};
u8 PlayerFx_ColorKeys_58[32] = {
#include "game/008D9E10.inc"
};
u8 PlayerFx_ColorKeys_59[32] = {
#include "game/008D9E30.inc"
};
u8 PlayerFx_BombRingSmokePartTypes[24] = {
#include "game/008D9E50.inc"
};
s16 PlayerFx_Durations_14[6] = {
#include "game/008D9E68.inc"
};
u32 PlayerFx_Curves_48[8] = {
#include "game/008D9E80.inc"
};
u8 PlayerFx_ColorKeys_61[32] = {
#include "game/008D9EA0.inc"
};
u16 PlayerFx_DrawTbl_15[4] = {
#include "game/008D9EC0.inc"
};
s16 PlayerFx_Durations_15[4] = {
#include "game/008D9EC8.inc"
};
u32 PlayerFx_Curves_50[12] = {
#include "game/008D9ED0.inc"
};
u32 PlayerFx_Curves_51[12] = {
#include "game/008D9F00.inc"
};
u8 PlayerFx_ColorKeys_63[32] = {
#include "game/008D9F30.inc"
};
u8 PlayerFx_ColorKeys_64[32] = {
#include "game/008D9F50.inc"
};
s16 PlayerFx_Durations_16[8] = {
#include "game/008D9F70.inc"
};
void *PlayerFx_Curves[52] = {
    (void *)PlayerFx_Curves_0, (void *)PlayerFx_Curves_1, (void *)PlayerFx_Curves_2, (void *)PlayerFx_Curves_3,
    (void *)PlayerFx_Curves_4, (void *)PlayerFx_Curves_5, (void *)PlayerFx_Curves_6, (void *)PlayerFx_Curves_7,
    (void *)PlayerFx_Curves_8, (void *)PlayerFx_Curves_9, (void *)PlayerFx_Curves_10, (void *)PlayerFx_Curves_11,
    (void *)PlayerFx_Curves_12, (void *)PlayerFx_Curves_13, (void *)PlayerFx_Curves_14, (void *)PlayerFx_Curves_15,
    (void *)PlayerFx_Curves_16, (void *)PlayerFx_Curves_17, (void *)PlayerFx_Curves_18, (void *)PlayerFx_Curves_19,
    (void *)PlayerFx_Curves_20, (void *)PlayerFx_Curves_21, (void *)PlayerFx_Curves_22, (void *)PlayerFx_Curves_23,
    (void *)PlayerFx_Curves_24, (void *)PlayerFx_Curves_25, (void *)PlayerFx_Curves_26, (void *)PlayerFx_Curves_27,
    (void *)PlayerFx_Curves_28, (void *)PlayerFx_Curves_29, (void *)PlayerFx_Curves_30, (void *)PlayerFx_Curves_31,
    (void *)PlayerFx_Curves_32, (void *)PlayerFx_Curves_33, (void *)PlayerFx_Curves_34, (void *)PlayerFx_Curves_35,
    (void *)PlayerFx_Curves_36, (void *)PlayerFx_Curves_37, (void *)PlayerFx_Curves_38, (void *)PlayerFx_Curves_39,
    (void *)PlayerFx_Curves_39, (void *)PlayerFx_Curves_41, (void *)PlayerFx_Curves_42, (void *)PlayerFx_Curves_43,
    (void *)PlayerFx_Curves_44, (void *)PlayerFx_Curves_45, (void *)PlayerFx_Curves_46, (void *)PlayerFx_Curves_46,
    (void *)PlayerFx_Curves_48, (void *)PlayerFx_Curves_48, (void *)PlayerFx_Curves_50, (void *)PlayerFx_Curves_51,
};
u8 *PlayerFx_ColorKeys[68] = {
    PlayerFx_ColorKeys_0, PlayerFx_ColorKeys_1, PlayerFx_ColorKeys_2, PlayerFx_ColorKeys_3,
    PlayerFx_ColorKeys_4, PlayerFx_ColorKeys_5, PlayerFx_ColorKeys_6, PlayerFx_ColorKeys_7,
    PlayerFx_ColorKeys_8, PlayerFx_ColorKeys_9, PlayerFx_ColorKeys_10, PlayerFx_ColorKeys_11,
    PlayerFx_ColorKeys_12, PlayerFx_ColorKeys_13, PlayerFx_ColorKeys_14, PlayerFx_ColorKeys_15,
    PlayerFx_ColorKeys_16, PlayerFx_ColorKeys_17, PlayerFx_ColorKeys_18, PlayerFx_ColorKeys_19,
    PlayerFx_ColorKeys_20, PlayerFx_ColorKeys_21, PlayerFx_ColorKeys_22, PlayerFx_ColorKeys_23,
    PlayerFx_ColorKeys_24, PlayerFx_ColorKeys_25, PlayerFx_ColorKeys_26, PlayerFx_ColorKeys_27,
    PlayerFx_ColorKeys_28, PlayerFx_ColorKeys_29, PlayerFx_ColorKeys_30, PlayerFx_ColorKeys_31,
    PlayerFx_ColorKeys_32, PlayerFx_ColorKeys_33, PlayerFx_ColorKeys_34, PlayerFx_ColorKeys_35,
    PlayerFx_ColorKeys_36, PlayerFx_ColorKeys_37, PlayerFx_ColorKeys_38, PlayerFx_ColorKeys_39,
    PlayerFx_ColorKeys_40, PlayerFx_ColorKeys_41, PlayerFx_ColorKeys_42, PlayerFx_ColorKeys_43,
    PlayerFx_ColorKeys_44, PlayerFx_ColorKeys_45, PlayerFx_ColorKeys_46, PlayerFx_ColorKeys_47,
    PlayerFx_ColorKeys_48, PlayerFx_ColorKeys_49, PlayerFx_ColorKeys_50, PlayerFx_ColorKeys_51,
    PlayerFx_ColorKeys_52, PlayerFx_ColorKeys_52, PlayerFx_ColorKeys_54, PlayerFx_ColorKeys_55,
    PlayerFx_ColorKeys_56, PlayerFx_ColorKeys_57, PlayerFx_ColorKeys_58, PlayerFx_ColorKeys_59,
    PlayerFx_ColorKeys_59, PlayerFx_ColorKeys_61, PlayerFx_ColorKeys_61, PlayerFx_ColorKeys_63,
    PlayerFx_ColorKeys_64, NULL, NULL, NULL,
};
s16 PlayerFx_PartCounts[24] = {
#include "game/008DA160.inc"
};
s16 PlayerFx_Lifetimes[24] = {
#include "game/008DA190.inc"
};
s16 PlayerFx_FirstCurve[24] = {
#include "game/008DA1C0.inc"
};
s16 PlayerFx_FirstColorKeys[24] = {
#include "game/008DA1F0.inc"
};
u16 *PlayerFx_MeshTbl[20] = {
    PlayerFx_DrawTbl_0, (u16 *)&PlayerFx_ColorKeys_10[24], PlayerFx_DrawTbl_2, PlayerFx_DrawTbl_3,
    PlayerFx_DrawTbl_0, PlayerFx_DrawTbl_5, PlayerFx_DrawTbl_6, PlayerFx_DrawTbl_7,
    PlayerFx_DrawTbl_8, PlayerFx_DrawTbl_6, PlayerFx_DrawTbl_10, PlayerFx_DrawTbl_11,
    PlayerFx_DrawTbl_12, PlayerFx_DrawTbl_13, PlayerFx_DrawTbl_14, PlayerFx_DrawTbl_15,
    (u16 *)D_008F5CE8, NULL, NULL, NULL,
};
s16 *PlayerFx_Durations[36] = {
    PlayerFx_SlashDurations, (s16 *)&PlayerFx_ColorKeys_10[40], PlayerFx_BowChargeDurations, PlayerFx_BowShotDurations,
    PlayerFx_SlashDurations, PlayerFx_Durations_5, PlayerFx_Durations_6, PlayerFx_HornNoteDurations,
    PlayerFx_GreatSwordChargeDurations, PlayerFx_Durations_6, PlayerFx_ImpactDurations, PlayerFx_Durations_11,
    PlayerFx_Durations_12, (s16 *)PlayerFx_Durations_13, PlayerFx_Durations_14, PlayerFx_Durations_15,
    PlayerFx_Durations_16, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_008DA300[8] = {
#include "game/008DA300.inc"
};
u32 D_008DA320[8] = {
#include "game/008DA320.inc"
};
u32 D_008DA340[12] = {
#include "game/008DA340.inc"
};
u32 D_008DA370[12] = {
#include "game/008DA370.inc"
};
u32 D_008DA3A0[12] = {
#include "game/008DA3A0.inc"
};
u32 D_008DA3D0[8] = {
#include "game/008DA3D0.inc"
};
u32 D_008DA3F0[8] = {
#include "game/008DA3F0.inc"
};
u32 D_008DA410[8] = {
#include "game/008DA410.inc"
};
u32 D_008DA430[12] = {
#include "game/008DA430.inc"
};
u32 D_008DA460[12] = {
#include "game/008DA460.inc"
};
u32 D_008DA490[8] = {
#include "game/008DA490.inc"
};
u32 D_008DA4B0[8] = {
#include "game/008DA4B0.inc"
};
u32 D_008DA4D0[12] = {
#include "game/008DA4D0.inc"
};
u32 D_008DA500[8] = {
#include "game/008DA500.inc"
};
u32 D_008DA520[12] = {
#include "game/008DA520.inc"
};
u32 D_008DA550[8] = {
#include "game/008DA550.inc"
};
