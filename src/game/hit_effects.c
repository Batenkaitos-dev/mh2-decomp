/*
 * The stage objects of five kinds, each with its update (a switch on
 * StageObj.state: set-up, run, end, free), the
 *
 * draw callback of its draw slots and its creators: ItemProp (an item's smoke
 * screen, rising clouds, a bait put down,
 *
 * a dropped carried item, an eating puff; here its puffs' states, its end and
 * free states and its creators), FireBlast
 *
 * (bomb blasts and their smoke, fires, flashes, the special mine point's
 * glow), ElementBurst (an elemental or status
 *
 * hit's burst, rings), HitSplash (the spray, flashes and sparks where a hit
 * lands) and AttackBurst (the monsters' shot
 *
 * flares, trails and impacts; here all its types but the rocks). An object
 * keeps its parts in its buffer blocks, one
 *
 * draw slot each; a part counts up from a negative frame, takes its scale and
 * opacity from the kind's keyed curves by
 *
 * its frame (Key_LerpVec3, Key_LerpFloat), is placed (mostly toward the
 * camera) and drawn with Effect_DrawModel. The
 *
 * kinds' tables follow the code; a curve array's number is its place in the
 * kind's curve table.
 */
#include "common.h"
#include "sprite.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "objattach.h"
#include "capsule.h"
#include "scene.h"
/* Declarations of the functions called below. */
void Vec_Copy(f32 *out, f32 *in);
void DrawSlot_Free(s32 i);
void DrawSlotB_Free(s32);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u8 Player_InCurrentArea(void *obj);

typedef struct ElementBurstPart {
    s16 kind;                   /* 0x00: its index; type 9: 1 */
    s16 slot;                   /* 0x02: its draw slot */
    f32 pos[3];                 /* 0x04 */
    f32 sc[3];                  /* 0x10: its scale, from a curve by its frame */
    f32 scale;                  /* 0x1C */
    DrawSlot *node;                /* 0x20 */
    s16 frame;                  /* 0x24: its frame, from a delay (negative) */
    u16 angle;                  /* 0x26: a random angle */
    s16 spin;                  /* 0x28: its spin */
    u8 alpha;                   /* 0x2A: from a curve by its frame */
    u8 unk2B;
} ElementBurstPart;

/* a part of a FireBlast (0x34 bytes, in its blocks) */
typedef struct FireBlastPart {
    s16 cell;                  /* 0x00: a random 0..31 */
    s16 frame;                  /* 0x02: counts up from -9 or -15 */
    s16 slot;                   /* 0x04: its draw slot */
    s16 look;                  /* 0x06 */
    u16 angle;                  /* 0x08: a random angle */
    s16 angleStep;                  /* 0x0A: its speed, +-0x5B */
    f32 curveScale[3];               /* 0x0C */
    f32 offset[3];               /* 0x18 */
    f32 alpha;                  /* 0x24: its opacity, 0 to 1, from a curve by its frame */
    f32 size;                  /* 0x28: a scale */
    f32 rise;                  /* 0x2C */
    DrawSlot *node;                /* 0x30 */
} FireBlastPart;

/* a puff of an ItemProp of types 5 and 6 (0x34 bytes, in its blocks) */
typedef struct ItemPropPuff {
    s16 slot;                   /* 0x00: its draw slot */
    s16 idx;                    /* 0x02 */
    f32 curveScale[3];               /* 0x04: its shape, from a curve by its frame (its scale is size times it) */
    f32 size;                  /* 0x10: its size: 4 to 8 (type 5), 1.6 to 2.2 (type 6) */
    f32 alpha;                  /* 0x14 */
    f32 fade;                  /* 0x18: its opacity factor: 1, fading to 0 after frame 90 (type 5); 0.15 (type 6) */
    s16 delay;                  /* 0x1C: frames before it starts (-5 per part) */
    u16 angle;                  /* 0x1E: a random angle */
    DrawSlot *node;                /* 0x20 */
    f32 pos[3];                 /* 0x24 */
    s16 spin;                   /* 0x30 */
    u8 unk32[0x34 - 0x32];
} ItemPropPuff;

/* a colour (r, g, b, and an alpha that the mine glow's colours use) */
typedef struct ColorRgba {
    u8 r;
    u8 g;
    u8 b;
    u8 alpha;
} ColorRgba;

/* a part of a HitSplash (0x34 bytes, in its blocks) */
typedef struct HitSplashPart {
    s16 slot;                   /* 0x00: its draw slot */
    s16 delay;                  /* 0x02: frames before it starts (negative) */
    f32 pos[3];                 /* 0x04 */
    f32 sc[3];                  /* 0x10: its scale */
    s16 idx;                    /* 0x1C: its index; 0xFF hides it (its owner gone); types 9, 12, 13: a random 0-7 picking its spin and model */
    u16 angle;                  /* 0x1E */
    u16 alpha;                  /* 0x20: 0xFF */
    u8 unk22[0x24 - 0x22];
    f32 scale;                  /* 0x24 */
    f32 fall;                  /* 0x28: its fall a frame: -0.8 (type 0) */
    DrawSlot *node;                /* 0x2C */
    s16 cell;                  /* 0x30 */
    u8 unk32[0x34 - 0x32];
} HitSplashPart;

/* a step of a HitSplash spray (type 0): at frame t, a burst of size v tenths */
typedef struct HitSplashStep {
    s16 frame;
    s16 size;
} HitSplashStep;

extern s16 gAreaFadeTimer;    /* the frames left in the fade's phase */
extern s16 gAreaFade;         /* the area's full-screen fade: 0 none, then its phases 1 to 3 */

/* prototypes */
extern u8 ElementBurst_PoisonColorKeys[];
extern u8 ElementBurst_WaterRingColorKeys[];
extern u8 ElementBurst_StatusColorKeys[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
typedef struct FireBlastBuf {
    FireBlastPart part[7];          /* 0x000 */
    EmObj flash;                /* 0x16C: its flash (FlashObj_Add): 5 frames */
} FireBlastBuf;
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 FlashObj_Add(EmObj *obj);
void FireBlast_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
extern void *FireBlast_PartCurves[];
extern s16 FireBlast_BlastCellFrames[];
extern s16 FireBlast_SmallBlastCellFrames[];
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
s32 SpecialMine_Rank(void);
extern s16 D_008CEEA8[];
void HitSplash_PlaySound(StageObj *o, f32 *pos);
void HitSplash_Draw(DrawSlot *node);
s32 Effect_OwnerColorRow(s32 a);
void Effect_ScaleVec(f32 *scale);
extern ColorRgba gEffectColors[];
void Mtx_RotateX(f32 *m, f32 a);
extern void *ItemProp_PuffCurves[];
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void ItemProp_OffsetFromBone(f32 *out, f32 *m, f32 *v);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
extern s16 ElementBurst_Limits[];
extern s16 ElementBurst_Steps[];
extern s16 ElementBurst_CurveIdx[];
extern s16 ElementBurst_Durations[];
extern s16 *ElementBurst_DurLists[];
extern void *ElementBurst_Curves[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Em_BonePos(void *obj, s16 bone, f32 *out);
s32 ElementBurst_RepeatRing(StageObj *o, ElementBurstPart *w);
s32 ElementBurst_RoarBone(StageObj *o);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
extern s16 ElementBurst_FireDelays[];
extern s16 ElementBurst_WaterDelays[];
extern s16 ElementBurst_RingDelays[];
void ElementBurst_PlaySound(StageObj *o);
void ElementBurst_Draw(DrawSlot *node);
extern s16 ItemProp_BaitStatusAmounts[];
s32 Player_IsLocal(void *obj);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
s32 ObjList_Add(EmObj *obj);
void Vec_RotateY(f32 *v, f32 a);
extern ColorRgba ItemProp_SmokeColors[];
extern f32 gCamMatrix[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
void ItemProp_Draw(DrawSlot *node);
void ItemProp_Run(StageObj *o);
void ItemProp_PlaySound(StageObj *o, s16 i);
EmBone *Em_Bone(void *obj, s16 bone);
extern u32 FireBlast_FlameShapes[];
extern f32 D_008CEE84[]; /* the second value of each pair of FireBlast_FlameShapes (4 bytes in) */
f32 Rand_Signed(void);
extern f32 HitSplash_TargetScales[];
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void FireBlast_Draw(DrawSlot *node);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void ItemProp_Create(f32 *pos, s16 v, s32 k, u8 c);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Inverse(f32 *out, f32 *m);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_RotateY(f32 *v, f32 angle);
u16 Rand_Next(s32 stream);
void FireBlast_Setup(StageObj *o);
void FireBlast_SetupParts(StageObj *o);
void FireBlast_SetupGlow(StageObj *o);
void FireBlast_Run(StageObj *o);
void FireBlast_RunParts(StageObj *o);
void FireBlast_RunGlow(StageObj *o);
void FireBlast_End(StageObj *o);
void FireBlast_Free(StageObj *o);
void ElementBurst_Setup(StageObj *o);
void ElementBurst_Run(StageObj *o);
void ElementBurst_End(StageObj *o);
void ElementBurst_Free(StageObj *o);
StageObj *ElementBurst_CreateAt(f32 *, s32, s32, f32);
void HitSplash_Setup(StageObj *o);
void HitSplash_Run(StageObj *o);
void HitSplash_End(StageObj *o);
void HitSplash_Free(StageObj *o);
void AttackBurst_Setup(StageObj *o);
void AttackBurst_Run(StageObj *o);
void AttackBurst_End(StageObj *o);
void AttackBurst_Free(StageObj *o);
void AttackBurst_RunRocks(StageObj *o);
void AttackBurst_RunParts(StageObj *o);
void AttackBurst_EndRocks(StageObj *o);
void AttackBurst_EndParts(StageObj *o);
void ItemProp_Update(StageObj *o);
extern s16 ElementBurst_PartCounts[];
extern s16 HitSplash_PartCounts[];
extern s16 FireBlast_PartCounts[];
void FlashObj_Remove(EmObj *obj);
void AttackBurst_SetupRocks(StageObj *o);
void AttackBurst_SetupParts(StageObj *o);
void AttackBurst_DrawPart(DrawSlot *node);
void AttackBurst_DrawRock(DrawSlot *node);
StageObj *ItemProp_Alloc(s32 kind);
f32 Math_Sin(f32 x);
void Gfx_SetBlend(s32 a, s32 b);
extern s16 FireBlast_MineGlowModels[];
extern ColorRgba FireBlast_MineRankColors[];
s16 EmSlot_FindKind(s16 id);
void Draw_SetDefaultState(void);
void Mtx_TurnY(f32 *m, f32 a);
extern u8 AttackBurst_Curve3A[];
extern u8 AttackBurst_ColorKeys[];
extern u8 AttackBurst_Curve3B[];
extern u8 AttackBurst_Curve10A[];
extern u8 AttackBurst_Curve10B[];
extern u8 AttackBurst_CurveFA[];
extern u8 AttackBurst_CurveFB[];
extern u8 AttackBurst_CurveFC[];
extern u8 AttackBurst_Curve12A[];
extern u8 AttackBurst_Curve12B[];
extern s16 HitSplash_CellFrames[];
extern u32 HitSplash_Curves[];
extern HitSplashStep HitSplash_SpraySteps[];
extern HitSplashStep HitSplash_SprayStepsStrong[];
extern HitSplashStep HitSplash_SprayStepsAtOnce[];
extern s16 HitSplash_Lives[];
extern s16 HitSplash_CurveIdx[];
extern s16 HitSplash_Ends[];
extern s16 HitSplash_Strides[];
f32 Effect_Scale(void);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
s32 HitSplash_SpinSpeed(s16 x);
/* end of prototypes */

/*
 * Sets up an ItemProp of type 5 (the rising cloud) or 6 (the eating puff).
 * Type 5 plays its first sound (ItemProp_PlaySound cue 0) and readies 6
 * puffs, each 5 frames after the last; type 6 one puff starting at once, with
 * a random spin, a size of 1.6 to 2.2 and a fade of 0.15. Each puff starts at
 * the object with a random angle and gets a draw slot that ItemProp_Draw
 * draws (none when no slot is free). Then runs the first frame
 * (ItemProp_Run).
 */
void ItemProp_SetupPuffs(StageObj *obj)
{
    ItemPropPuff *puff = (ItemPropPuff *) obj->buf;
    s16 count;
    s16 i;

    obj->draw = NULL;
    obj->timer = 0;
    obj->angle = 0;
    switch (obj->type) {
    case ITEMPROP_CLOUD:
    default:
        ItemProp_PlaySound(obj, 0);
        count = 6;
        break;
    case ITEMPROP_EAT_PUFF:
        count = 1;
        break;
    }
    for (i = 0; i < count; i++, puff++) {
        switch (obj->type) {
        case ITEMPROP_CLOUD:
        default:
            puff->idx = i;
            puff->delay = -i * 5;
            puff->angle = Rand_Next(1);
            Vec_Copy(puff->pos, obj->pos);
            break;
        case ITEMPROP_EAT_PUFF:
            puff->idx = i;
            puff->delay = 0;
            puff->angle = Rand_Next(1);
            puff->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            puff->fade = 0.15f;
            puff->size = 1.6f + 0.6f * (0.001f * (s32) (Rand_Next(1) & 0x3FF));
            Vec_Copy(puff->pos, obj->pos);
            break;
        }
        puff->slot = DrawSlot_Alloc();
        if (puff->slot != -1) {
            puff->node = DrawSlot_Get(puff->slot);
            puff->node->owner = obj;
            puff->node->part = i;
            puff->node->draw = ItemProp_Draw;
        } else {
            puff->node = NULL;
        }
    }
    ItemProp_Run(obj);
}

/*
 * Runs the puffs of types 5 and 6 for a frame. Type 5 lives 126 frames and
 * plays its second sound every 20 frames before frame 90; a puff, on
 * starting, takes a size from the timer (4, growing to 8 at frame 50), a fade
 * dropping from 1 to 0 over frames 90 to 105, a new angle and spin and the
 * object's position, and after 21 frames starts again 9 frames later as the
 * next puff (idx + 6) until frame 105. Type 6 lives 60 frames. A live puff
 * takes its shape and opacity from ItemProp_PuffCurves by its frame, rises
 * (type 5 by its size, type 6 by a random share of it), turns, and its slot
 * goes into the sorted draw table. At the end of its life the object goes to
 * the end state, hidden.
 */
void ItemProp_RunPuffs(StageObj *obj)
{
    ItemPropPuff *puff = (ItemPropPuff *) obj->buf;
    s16 i;
    s16 dur;
    s16 limit;
    s16 count;
    void *curve;
    s16 curveIdx;

    switch (obj->type) {
    case ITEMPROP_CLOUD:
    default:
        count = 6;
        limit = 0x7E;
        dur = 0x15;
        if (obj->timer < 90 && obj->timer % 20 == 0) {
            ItemProp_PlaySound(obj, 1);
        }
        break;
    case ITEMPROP_EAT_PUFF:
        limit = 60;
        count = 1;
        dur = 60;
        break;
    }
    if (++obj->timer > limit) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < count; i++) {
        switch (obj->type) {
        case ITEMPROP_CLOUD:
        default:
            curveIdx = 0;
            break;
        case ITEMPROP_EAT_PUFF:
            curveIdx = 2;
            break;
        }
        if (++puff->delay <= 0) {
            puff++;
            continue;
        }
        if (puff->delay == 1) {
            if (obj->type == ITEMPROP_CLOUD) {
                if (obj->timer < 50) {
                    puff->size = 4.0f + 4.0f * (obj->timer / 50.0f);
                } else {
                    puff->size = 8.0f;
                }
                if (obj->timer < 90) {
                    puff->fade = 1.0f;
                } else {
                    puff->fade = 1.0f - (obj->timer - 90) / 15.0f;
                    if (puff->fade < 0.0f) {
                        puff->fade = 0.0f;
                    }
                }
                puff->angle = Rand_Next(1);
                puff->spin = 0.5f + (65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200))) / 360.0f;
                Vec_Copy(puff->pos, obj->pos);
            }
        } else if (puff->delay > dur) {
            if (obj->type == ITEMPROP_CLOUD && obj->timer < 105) {
                puff->delay = -9;
                puff->idx += 6;
            }
            puff++;
            continue;
        }
        curve = ItemProp_PuffCurves[curveIdx++];
        Key_LerpVec3(curve, puff->curveScale, puff->delay);
        Key_LerpFloat(ItemProp_PuffCurves[curveIdx], &puff->alpha, puff->delay);
        puff->alpha *= puff->fade;
        switch (obj->type) {
        case ITEMPROP_CLOUD:
        default:
            puff->pos[1] += (40.0f * puff->size) / 21.0f;
            break;
        case ITEMPROP_EAT_PUFF:
            puff->pos[1] += puff->size * (0.5f + 0.00025f * (Rand_Next(1) & 0x3FF));
            break;
        }
        puff->angle += puff->spin;
        if (puff->node != NULL) {
            Vec_Copy(puff->node->pos, puff->pos);
            OT_AddByDepth(gDrawOtSorted, puff->node, 0x40, 0);
        }
        puff++;
    }
}

/*
 * Draws one puff (node->part) of types 5 and 6: stage model 72 in the colour
 * ItemProp_SmokeColors[var1] for type 5, model 74 in white for type 6, scaled
 * by its size times its shape and turned about the view axis by its angle,
 * its opacity as the colour's alpha. Depth writes are off while it draws;
 * nothing is drawn while the stage model set is not loaded.
 */
void ItemProp_DrawPuff(StageObj *obj, DrawSlot *node)
{
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    ItemPropPuff *puff = (ItemPropPuff *) obj->buf + node->part;
    ModelMat *mats;
    s32 color;
    u8 opacity;
    ModelMatSet *model;
    Model *set = gStageModelSet;

    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Gs_SetState(0x60, 0);
        Gs_SetState(0x6C, 0);
        scale[0] = puff->size * puff->curveScale[0];
        scale[1] = puff->size * puff->curveScale[1];
        scale[2] = puff->size * puff->curveScale[2];
        rot[2] = (6.2831855f * puff->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        opacity = 255.0f * puff->alpha;
        switch (obj->type) {
        case ITEMPROP_CLOUD:
        default:
            model = &set->matSets[72];
            color = ItemProp_SmokeColors[obj->var1].b
                    | ((ItemProp_SmokeColors[obj->var1].g << 8) | ((opacity << 24) | (ItemProp_SmokeColors[obj->var1].r << 16)));
            break;
        case ITEMPROP_EAT_PUFF:
            model = &set->matSets[74];
            color = (opacity << 24) | 0xFFFFFF;
            break;
        }
        Mtx_Concat3x3(mtx, gCamMatrix);
        Effect_DrawModel(model, mtx, color, 0, mats);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * The end state: hides the object, moves on to the free state and frees its
 * draw slots: types 1, 5 and 6 those of their 4, 6 or 1 parts (0x34 bytes
 * each; a part whose node is NULL has none), type 3 its own from the second
 * pool (DrawSlotB_Free), the other types their own slot.
 */
void ItemProp_End(StageObj *obj)
{
    u8 *piece = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    obj->visible = 0;
    switch (obj->type) {
    default:
        DrawSlot_Free(obj->slot);
        return;
    case ITEMPROP_SMOKE:
        count = 4;
        break;
    case ITEMPROP_BAIT:
        DrawSlotB_Free(obj->slot);
        return;
    case ITEMPROP_CLOUD:
        count = 6;
        break;
    case ITEMPROP_EAT_PUFF:
        count = 1;
        break;
    }
    for (i = 0; i < count; i++) {
        if (*(s32 *) (piece + 0x20) != 0) {
            DrawSlot_Free(*(s16 *) piece);
        }
        piece += 0x34;
    }
}

/*
 * The free state: when a bait (type 3) of the local player goes, one fewer is
 * counted in gScene.placed3; then frees the object.
 */
void ItemProp_Free(StageObj *obj)
{
    if (obj->type == ITEMPROP_BAIT && obj->ownerIndex == gScene.localPlayer) {
        (gScene.placed3)--;
    }
    StageObj_Free(obj);
}

/*
 * Plays type 5's sounds at the object from the common bank: cue 0 its burst
 * at the set-up, cue 1 the sound repeated while it rises. The other types
 * have none.
 */
void ItemProp_PlaySound(StageObj *obj, s16 cue)
{
    switch (obj->type) {
    case ITEMPROP_CLOUD:
        switch (cue) {
        case 0:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, obj->pos, 1, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(1, 0x27, 0, obj->pos, 1, 0);
            break;
        }
        break;
    }
}

/*
 * Allocates an ItemProp of the given type: one block of the object buffer for
 * types 1 to 6 (their parts, the bait's goal object or the dropped item's
 * matrix), none for type 0 and the others. Returns the object, or NULL when
 * none is free.
 */
StageObj *ItemProp_Alloc(s32 type)
{
    switch ((s16) type) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 1:
        return StageObj_Alloc(1);
    case 0:
    default:
        return StageObj_Alloc(0);
    }
}

s32 Obj_InCurrentArea(void *obj);
/*
 * Creates an ItemProp of type k for hunter pl when pl is in the current area
 * (type 3 in any area); it takes pl's area and yaw. Type 3 (a bait) sits 50
 * ahead and 13 up, kept when the area reloads and counted in gScene.placed3
 * for the local player. Type 4 (the item pl carried, dropped) takes pl's
 * carry kind and sits at pl's bone 0xE, moved by an offset and given a scale
 * and rotation per carry kind (an unknown kind returns with them unset). Type
 * 6 (an eating puff) sits halfway between bones 0xE and 0x12, 15 higher; the
 * other types at pl's position.
 */
void ItemProp_CreateAtPlayer(Player *pl, s32 k)
{
    StageObj *obj;
    ItemPropCarry *held;
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 boneMtx[16];
    f32 offset[3];

    if ((s16) k == 3 || (Obj_InCurrentArea(pl) & 0xFF)) {
        obj = ItemProp_Alloc(k);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_ITEM_PROP;
            obj->update = ItemProp_Update;
            obj->type = k;
            obj->owner = pl;
            obj->angle2 = pl->area;
            obj->angle = pl->rot[1];
            switch ((s16) k) {
            case 3:
                Vec_Copy(obj->pos, pl->pos);
                obj->var1 = 0;
                offset[0] = 0.0f;
                offset[1] = 13.0f;
                offset[2] = 50.0f;
                Vec_RotateY(offset, (6.2831855f * obj->angle) / 65536.0f);
                obj->pos[0] += offset[0];
                obj->pos[1] += offset[1];
                obj->pos[2] += offset[2];
                obj->keep = 1;
                obj->ownerIndex = ((Player *) obj->owner)->index;
                if (Player_IsLocal(pl)) {
                    (gScene.placed3)++;
                }
                break;
            case 4:
                held = (ItemPropCarry *) obj->buf;
                held->kind = (pl->carry & PL_CARRY_KIND_MASK) >> PL_CARRY_KIND_SHIFT;
                switch (held->kind) {
                case 0:
                case 1:
                case 6:
                    scaleX = 1.0f;
                    scaleY = 1.3f;
                    scaleZ = 1.0f;
                    rotX = -0.9424778f;
                    rotY = -3.2812192f;
                    rotZ = 2.0594885f;
                    offset[0] = 5.5f;
                    offset[1] = -28.0f;
                    offset[2] = 12.0f;
                    break;
                case 2:
                    scaleX = 1.0f;
                    scaleY = 1.0f;
                    scaleZ = 1.0f;
                    rotX = -0.9424778f;
                    rotY = -3.2812192f;
                    rotZ = 2.0594885f;
                    offset[0] = 8.9f;
                    offset[1] = -25.0f;
                    offset[2] = 4.6f;
                    break;
                case 3:
                case 4:
                case 5:
                    scaleX = 2.5f;
                    scaleY = 3.0f;
                    scaleZ = 2.0f;
                    rotX = -0.24434611f;
                    rotY = 2.86234f;
                    rotZ = 1.2566371f;
                    offset[0] = 3.3f;
                    offset[1] = -24.0f;
                    offset[2] = 4.0f;
                    break;
                default:
                    return;
                }
                Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(pl, 0xE)));
                ItemProp_OffsetFromBone(obj->pos, boneMtx, offset);
                Mtx_Scale(held->mtx, scaleX, scaleY, scaleZ);
                Mtx_RotateXYZ(held->mtx, rotX, rotY, rotZ);
                Mtx_Concat3x3(held->mtx, boneMtx);
                break;
            case 6:
                Em_GetBonePos(pl, 0xE, obj->pos);
                Em_GetBonePos(pl, 0x12, offset);
                obj->pos[0] += offset[0];
                obj->pos[1] += offset[1];
                obj->pos[2] += offset[2];
                obj->pos[0] *= 0.5f;
                obj->pos[1] *= 0.5f;
                obj->pos[2] *= 0.5f;
                obj->pos[1] += 15.0f;
                break;
            default:
                Vec_Copy(obj->pos, pl->pos);
                break;
            }
        }
    }
}

/*
 * Creates an ItemProp of type k at the attack object's position when that
 * object is in the current area, with colour row colorRow (var1) and the
 * object's area. A smoke screen (type 1) also makes a cloud (type 5) of its
 * colour there.
 */
void ItemProp_CreateFromAttach(ObjAttach *attack, s32 k, u8 colorRow)
{
    StageObj *obj;

    if (attack->area == gScene.area) {
        obj = ItemProp_Alloc(k);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_ITEM_PROP;
            obj->update = ItemProp_Update;
            obj->type = k;
            obj->pos[0] = attack->pos[0];
            obj->pos[1] = attack->pos[1];
            obj->pos[2] = attack->pos[2];
            obj->var1 = colorRow;
            obj->angle2 = attack->area;
            if (obj->type == ITEMPROP_SMOKE) {
                ItemProp_Create(obj->pos, 0, ITEMPROP_CLOUD, obj->var1);
            }
        }
    }
}

/*
 * Creates an ItemProp of type k at pos with the yaw and colour row colorRow
 * (var1), in any area.
 */
void ItemProp_Create(f32 *pos, s16 yaw, s32 k, u8 colorRow)
{
    StageObj *obj = ItemProp_Alloc(k);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_ITEM_PROP;
        obj->update = ItemProp_Update;
        obj->type = k;
        obj->var1 = colorRow;
        obj->angle = yaw;
        Vec_Copy(obj->pos, pos);
    }
}

/*
 * Creates the meat or bait (type 3) hunter pl puts down, baitKind (0 to 3, in
 * var1) saying which: 50 ahead of pl and 13 up, with pl's area and yaw, kept
 * when the area reloads. For type 3 it counts the local player's in
 * gScene.placed3 and adds its buffer's EmObj to the area object list
 * (ObjList_Add) for the monsters' goal search: radius 5000, at the object,
 * goalArg pl's index, goalNo pl's count of items used, kind baitKind, in pl's
 * area.
 */
void ItemProp_CreateBait(Player *pl, s32 k, u8 baitKind)
{
    StageObj *o;
    ItemPropBuf *bait;
    f32 offset[3];

    o = ItemProp_Alloc(k);
    if (o != NULL) {
        o->kind = STAGEOBJ_KIND_ITEM_PROP;
        o->update = ItemProp_Update;
        o->type = k;
        o->owner = pl;
        o->var1 = baitKind;
        o->angle2 = pl->area;
        o->angle = pl->rot[1];
        Vec_Copy(o->pos, pl->pos);
        offset[0] = 0.0f;
        offset[1] = 13.0f;
        offset[2] = 50.0f;
        Vec_RotateY(offset, (6.2831855f * o->angle) / 65536.0f);
        o->pos[0] += offset[0];
        o->pos[1] += offset[1];
        o->pos[2] += offset[2];
        o->keep = 1;
        if ((s16) k == 3) {
            o->ownerIndex = ((Player *) o->owner)->index;
            if (Player_IsLocal(pl)) {
                (gScene.placed3)++;
            }
            bait = (ItemPropBuf *) o->buf;
            bait->obj.radius = 5000.0f;
            Vec_Set(bait->obj.pos, o->pos[0], o->pos[1], o->pos[2]);
            bait->obj.goalArg = ((Player *) o->owner)->index;
            bait->obj.goalNo = gScene.itemUses[((Player *) o->owner)->index];
            bait->obj.done = 0;
            bait->obj.kind = o->var1;
            bait->obj.statusAmount = ItemProp_BaitStatusAmounts[o->var1];
            bait->obj.area = *(u8 *) &((Player *) o->owner)->area;
            ObjList_Add(&bait->obj);
        }
    }
}

/*
 * FireBlast's update, run every frame: switches on obj->state to its set-up,
 * run, end and free states.
 */
void FireBlast_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        FireBlast_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        FireBlast_Run(obj);
        break;
    case STAGEOBJ_END:
        FireBlast_End(obj);
        break;
    case STAGEOBJ_FREE:
        FireBlast_Free(obj);
        break;
    }
}

/*
 * The set-up state: visible, timer 0; the area fire's glow (type 2) gets its
 * one draw slot (FireBlast_SetupGlow), the other types their parts
 * (FireBlast_SetupParts).
 */
void FireBlast_Setup(StageObj *obj)
{
    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    if (obj->type != 2) {
        FireBlast_SetupParts(obj);
        return;
    }
    FireBlast_SetupGlow(obj);
}

/*
 * Starts or restarts a flame of a fire (types 3 and 12): frame -9, a random
 * texture cell (0 to 31), one of the five shapes of FireBlast_FlameShapes,
 * and a random offset from the object (x and z within 7.7, y 0 to 5).
 */
void FireBlast_InitFlame(FireBlastPart *flame)
{
    s16 shape;

    flame->frame = -9;
    flame->cell = Rand_Next(1) & 0x1F;
    shape = Rand_Next(1) % 5;
    flame->curveScale[0] = ((f32 (*)[2])FireBlast_FlameShapes)[shape][0];
    flame->curveScale[1] = ((f32 (*)[2])D_008CEE84)[shape][0];
    flame->curveScale[2] = 1.0f;
    flame->offset[0] = 0.015000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
    flame->offset[1] = 0.005f * (s32) (Rand_Next(1) & 0x3FF);
    flame->offset[2] = 0.015000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
}

/*
 * Restarts a flame of the spit-roast fire (type 6) as FireBlast_InitFlame
 * does, but keeps its frame and takes a narrower offset (x and z within 5.1,
 * y 0 to 2.6).
 */
void FireBlast_InitRoastFlame(FireBlastPart *flame)
{
    s16 shape;

    flame->cell = Rand_Next(1) & 0x1F;
    shape = Rand_Next(1) % 5;
    flame->curveScale[0] = ((f32 (*)[2])FireBlast_FlameShapes)[shape][0];
    flame->curveScale[1] = ((f32 (*)[2])D_008CEE84)[shape][0];
    flame->curveScale[2] = 1.0f;
    flame->offset[0] = 0.010000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
    flame->offset[1] = 0.0050000004f * (s32) (Rand_Next(1) & 0x3FF);
    flame->offset[2] = 0.010000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
}

/*
 * Starts or restarts a sparkle of the special mine point's glow (type 10): at
 * the object, x and z moved by up to 5 times its size, frame -15, one of two
 * looks and a random angle, turning 0x5B a frame one way or the other.
 */
void FireBlast_InitMineGlow(StageObj *obj, FireBlastPart *glow)
{
    u16 bits;

    Vec_Copy(glow->offset, obj->pos);
    bits = Rand_Next(1);
    glow->frame = -0xF;
    glow->offset[0] += obj->scale * (5.0f * Rand_Signed());
    glow->offset[2] += obj->scale * (5.0f * Rand_Signed());
    glow->look = bits & 1;
    glow->angle = bits;
    glow->angleStep = 0x5B;
    if ((bits >> 1) & 1) {
        glow->angleStep = -glow->angleStep;
    }
}

/*
 * Sets up the parts (FireBlast_PartCounts[type] of them), each with a draw
 * slot that FireBlast_Draw draws. The area fire (3) starts its crackle
 * (SoundObj_Start, 75 frames); the mine glow (10) keeps the point's rank in
 * var0 and restarts its colour fade (angle 0). Per type: the blasts (0, 11) a
 * core and two puffs, each puff hidden or 3 to 6 frames late at a random
 * offset; the small blast (1) puffs 2 frames apart, stacked toward the
 * camera; the fires (3, 12) a steady base and flames 5 frames apart
 * (FireBlast_InitFlame), the roast fire (6) its flames
 * (FireBlast_InitRoastFlame); the flash bomb (4) its four lights at
 * FireBlast_FlashBombDelays; the smoke (8) puffs behind the blast rising and
 * turning at random speeds; the mine glow sparkles 15 frames apart.
 */
void FireBlast_SetupParts(StageObj *obj)
{
    FireBlastPart *piece = (FireBlastPart *) obj->buf;
    s16 count = FireBlast_PartCounts[obj->type];
    s16 i;
    f32 riseBase;

    switch (obj->type) {
    case 3:
        SoundObj_Start(7, 0x10, 0x13, obj->pos, 0x4B, 2);
        break;
    case 10:
        obj->var0 = SpecialMine_Rank();
        obj->angle = 0;
        break;
    }
    for (i = 0; i < count; piece++, i++) {
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = FireBlast_Draw;
        } else {
            piece->node = NULL;
        }
        piece->frame = 0;
        piece->cell = 0;
        switch (obj->type) {
        case 0:
        case 11:
            piece->look = i;
            if (i == 0) {
                piece->offset[0] = 0.0f;
                piece->offset[1] = 0.0f;
                piece->offset[2] = 0.0f;
            }
            if (i == 1 || i == 2) {
                if (Rand_Next(1) & 1) {
                    piece->look = 0xFF;
                } else {
                    piece->frame = -((Rand_Next(1) & 3) + 3);
                    piece->offset[0] = 0.2f * obj->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
                    piece->offset[1] = 0.2f * obj->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
                    piece->offset[2] = 0.0f;
                }
            }
            piece->angle = Rand_Next(1);
            break;
        case 1:
            piece->look = i;
            piece->frame = -3 - i * 2;
            piece->offset[0] = obj->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            piece->offset[1] = obj->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            piece->offset[2] = 20.0f + i * 20;
            piece->size = obj->scale * (1.0f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            piece->angle = Rand_Next(1);
            break;
        case 3:
        case 12:
            if (i == 0) {
                piece->look = 0xFF;
                piece->frame = 0;
                piece->cell = Rand_Next(1) & 0xF;
                piece->curveScale[0] = 2.0f;
                piece->curveScale[1] = 2.0f;
                piece->curveScale[2] = 1.0f;
                piece->alpha = 1.0f;
                piece->offset[0] = 0.0f;
                piece->offset[1] = 0.0f;
                piece->offset[2] = 0.0f;
            } else {
                piece->look = i - 1;
                FireBlast_InitFlame(piece);
                piece->frame = -piece->look * 5;
            }
            piece->size = 1.0f;
            break;
        case 4:
            piece->frame = D_008CEEA8[i];
            if (i < 3) {
                piece->angle = 0;
                piece->look = i;
            } else {
                piece->angle = Rand_Next(1);
                piece->look = 3;
            }
            Vec_Copy(piece->offset, obj->pos);
            break;
        case 5:
            piece->look = i;
            Vec_Copy(piece->offset, obj->pos);
            break;
        case 6:
            piece->look = i - 1;
            FireBlast_InitRoastFlame(piece);
            piece->frame = -i * 5;
            piece->size = 1.0f;
            break;
        case 8:
            piece->frame = -6;
            piece->angle = Rand_Next(1);
            piece->look = Rand_Next(1) & 7;
            if (!(piece->look & 2)) {
                riseBase = 0.5f;
                piece->angleStep = (s32) (0.5f + (65536.0f * (0.375f * (0.001f * (Rand_Next(1) & 0x3FF)))) / 360.0f);
            } else {
                riseBase = 0.5f;
                piece->angleStep = (s32) (0.5f + (65536.0f * (0.5f * (0.001f * (Rand_Next(1) & 0x3FF)))) / 360.0f);
            }
            if (piece->look & 4) {
                piece->angleStep = -piece->angleStep;
            }
            piece->size = 0.7f + 0.0008f * (Rand_Next(1) & 0x3FF);
            piece->rise = riseBase + 0.0005f * (Rand_Next(1) & 0x3FF);
            piece->offset[0] = 0.2f * obj->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
            piece->offset[1] = 0.1f * obj->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
            piece->offset[2] = -30.0f - 5.0f * i;
            break;
        case 9:
            piece->look = i;
            piece->angle = Rand_Next(1);
            piece->alpha = 1.0f;
            break;
        case 10:
            FireBlast_InitMineGlow(obj, piece);
            piece->frame = -15 * i;
            piece->size = 1.0f;
            break;
        case 2:
        case 7:
        default:
            piece->look = i;
            break;
        }
    }
}

/*
 * Gives the area fire's glow (type 2) one draw slot at its position, drawn by
 * FireBlast_Draw; frees the object when no slot is free.
 */
void FireBlast_SetupGlow(StageObj *obj)
{
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = FireBlast_Draw;
        Vec_Copy(obj->node->pos, obj->pos);
    } else {
        StageObj_Free(obj);
    }
}

/* The run state: the glow (type 2) runs FireBlast_RunGlow, the other types FireBlast_RunParts. */
void FireBlast_Run(StageObj *obj)
{
    if (obj->type != 2) {
        FireBlast_RunParts(obj);
        return;
    }
    FireBlast_RunGlow(obj);
}

/*
 * Runs the parts for a frame. Each type has its life (35 frames the blasts,
 * 25 the small blast, 30 the flash bomb, 20 the flash, 36 the roast fire, 88
 * the smoke); the fires (3, 12) and the mine glow (10) never time out. The
 * flash bomb makes its flash (type 5) at frame 12. The flash, at frame 9,
 * starts the screen's fade (gAreaFade) and, when its owner is a hunter, adds
 * its EmObj to the monsters' flash list (FlashObj_Add, radius 2500), kept
 * alive each frame after. The mine glow ends when the special mine point is
 * gone and restarts its colour fade when the rank changes. The monster's
 * flame (12) follows bone 0xB of its owner, sounds every 25 frames and ends
 * with the owner. Each part waits out its delay, takes its shape and opacity
 * from FireBlast_PartCurves by its frame (the fires' flames and the glow's
 * sparkles restart when done), steps its texture cell (at
 * FireBlast_BlastCellFrames or FireBlast_SmallBlastCellFrames, each frame for
 * flames), is placed and goes into the sorted draw table.
 */
void FireBlast_RunParts(StageObj *obj)
{
    FireBlastPart *piece = (FireBlastPart *) obj->buf;
    s16 count = FireBlast_PartCounts[obj->type];
    u16 limit;
    s16 k;
    u16 dur;
    s16 i;
    s16 j;
    s16 nextCell;
    s32 rank;
    Em *em;
    f32 *boneMtx;
    f32 offset[3];
    void *curve;

    switch (obj->type) {
    case 0:
    case 11:
        limit = 0x23;
        break;
    case 1:
        limit = 0x19;
        dur = 0x12;
        break;
    case 10:
        if (gQuest.specialMinePoint == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        dur = limit = 0x1E;
        rank = SpecialMine_Rank();
        if (obj->var0 != rank) {
            obj->angle = 0;
            obj->var0 = rank;
        }
        if (obj->angle <= 15) {
            obj->angle++;
        }
        break;
    case 3:
        dur = limit = 0x1A;
        break;
    case 4:
        limit = 0x1E;
        if (obj->timer == 12) {
            FireBlast_CreateOwned(obj->pos, 5, obj->scale, obj->owner);
        }
        break;
    case 5:
        limit = 0x14;
        switch (obj->var0) {
        case 0:
            if (obj->timer >= 9) {
                em = obj->owner;
                if (em->isEm == 0) {
                    Vec_Copy(((FireBlastBuf *) piece)->flash.pos, obj->pos);
                    ((FireBlastBuf *) piece)->flash.radius = 2500.0f;
                    ((FireBlastBuf *) piece)->flash.goalArg = ((Em *) obj->owner)->index;
                    ((FireBlastBuf *) piece)->flash.timer = 5;
                    ((FireBlastBuf *) piece)->flash.area = gScene.area;
                    FlashObj_Add((EmObj *)((u8 *) piece + 0x16C));
                }
                obj->var0++;
                if (gAreaFade == 3) {
                    if (gAreaFadeTimer <= 50) {
                        gAreaFade = 1;
                        gAreaFadeTimer = 2;
                    }
                } else if (gAreaFade == 0) {
                    gAreaFade = 1;
                    gAreaFadeTimer = 2;
                }
            }
            break;
        case 1:
        default:
            ((FireBlastBuf *) piece)->flash.timer = 5;
            break;
        }
        break;
    case 6:
        limit = 0x24;
        dur = 0x1A;
        break;
    case 8:
        limit = 0x58;
        break;
    case 9:
        dur = limit = 2;
        break;
    case 12:
        em = obj->owner;
        if (em == NULL || em->active == 0 || em->taskPhase >= 2) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        dur = limit = 0x1A;
        boneMtx = (f32 *)Em_Bone(em, 0xB);
        offset[0] = 0.0f;
        offset[1] = 30.0f;
        offset[2] = 8.5f;
        Vec_RotateSelf(offset, boneMtx);
        Mtx_GetTrans(obj->pos, boneMtx);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        if (obj->timer % 25 == 0) {
            Em_PlaySeChecked(obj->owner, 0x33, 0, obj->pos, 1, 0);
        }
        break;
    }
    obj->timer++;
    if (obj->type != 3 && obj->type != 10 && obj->type != 12 && obj->timer > limit) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < count; i++) {
        switch (obj->type) {
        case 0:
        case 11:
            j = piece->look;
            switch (j) {
            case 0:
            case 1:
            case 2:
                k = 0;
                dur = 0x1B;
                break;
            case 3:
                k = 2;
                dur = 8;
                break;
            case 4:
                k = 4;
                dur = 9;
                break;
            case 0xFF:
                piece++;
                continue;
            }
            break;
        case 1:
            k = 0x12;
            break;
        case 3:
        case 6:
        case 12:
            k = 0x19;
            break;
        case 4:
            switch (piece->look) {
            case 0:
                k = 6;
                dur = 8;
                break;
            case 1:
                k = 8;
                dur = 6;
                break;
            case 2:
                k = 0xA;
                dur = 0xB;
                break;
            case 3:
                k = 0xC;
                dur = 0x15;
                break;
            }
            break;
        case 5:
            switch (piece->look) {
            case 0:
                k = 0xE;
                dur = 0x14;
                break;
            case 1:
            default:
                k = 0x10;
                dur = 0xE;
                break;
            }
            break;
        case 8:
            k = 0x14;
            if (!(piece->look & 2)) {
                dur = 0x50;
            } else {
                k = 0x16;
                dur = 0x3C;
            }
            break;
        case 9:
            k = 0x18;
            break;
        case 10:
            k = 0x1A;
            dur = 0x1E;
            break;
        }
        if (piece->look != 0xFF && ++piece->frame <= 0) {
            piece++;
            continue;
        }
        if (piece->look != 0xFF && piece->frame > dur) {
            if (obj->type == 3 || obj->type == 12) {
                FireBlast_InitFlame(piece);
            } else if (obj->type == 10) {
                FireBlast_InitMineGlow(obj, piece);
            }
            piece++;
            continue;
        }
        if (obj->type != 3 && obj->type != 6 && obj->type != 12) {
            curve = FireBlast_PartCurves[k++];
            Key_LerpVec3(curve, piece->curveScale, piece->frame);
        }
        if (obj->type != 9 && piece->look != 0xFF) {
            curve = FireBlast_PartCurves[k++];
            Key_LerpFloat(curve, &piece->alpha, piece->frame);
        }
        if ((obj->type == 0 || obj->type == 11) && ((u32) piece->look <= 1 || piece->look == 2)) {
            for (j = 0; FireBlast_BlastCellFrames[j] != -1; j++) {
                if (piece->frame == FireBlast_BlastCellFrames[j]) {
                    piece->cell++;
                    break;
                }
            }
        } else if (obj->type == 1) {
            for (j = 0; FireBlast_SmallBlastCellFrames[j] != -1; j++) {
                if (piece->frame == FireBlast_SmallBlastCellFrames[j]) {
                    piece->cell++;
                    break;
                }
            }
        } else if (obj->type == 3 || obj->type == 6 || obj->type == 12) {
            nextCell = piece->cell & 0xF;
            piece->cell &= ~0xF;
            nextCell++;
            piece->cell |= nextCell & 0xF;
        }
        if (piece->node != NULL) {
            switch (obj->type) {
            case 0:
            case 11:
                Vec_Copy(piece->node->pos, obj->pos);
                if (piece->look <= 2) {
                    offset[0] = piece->offset[0];
                    offset[1] = piece->offset[1];
                    offset[2] = 10.0f + piece->offset[2];
                } else if (piece->look == 3) {
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 20.0f;
                } else {
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = 0.0f;
                }
                Vec_RotateSelf(offset, gCamMatrix);
                piece->node->pos[0] += offset[0];
                piece->node->pos[1] += offset[1];
                piece->node->pos[2] += offset[2];
                break;
            case 1:
                Vec_Copy(piece->node->pos, obj->pos);
                Vec_Rotate(offset, piece->offset, gCamMatrix);
                piece->node->pos[0] += offset[0];
                piece->node->pos[1] += offset[1];
                piece->node->pos[2] += offset[2];
                break;
            case 3:
            case 6:
                piece->node->pos[0] = obj->pos[0] + piece->offset[0];
                piece->node->pos[1] = obj->pos[1] + piece->offset[1];
                piece->node->pos[2] = obj->pos[2] + piece->offset[2];
                break;
            case 4:
                if (piece->look == 2) {
                    piece->offset[1] -= 5.0f;
                }
            case 5:
                Vec_Copy(piece->node->pos, piece->offset);
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 10.0f + (15.0f - 5.0f * i);
                Vec_RotateSelf(offset, gCamMatrix);
                piece->node->pos[0] += offset[0];
                piece->node->pos[1] += offset[1];
                piece->node->pos[2] += offset[2];
                break;
            case 8:
                piece->offset[1] += piece->rise;
                piece->angle += piece->angleStep;
                Vec_Rotate(offset, piece->offset, gCamMatrix);
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
                break;
            case 9:
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = i * 5 + 25;
                Vec_RotateSelf(offset, gCamMatrix);
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
                break;
            case 10:
                piece->offset[1] += 2.0f * obj->scale;
                piece->angle += piece->angleStep;
                piece->node->pos[0] = piece->offset[0];
                piece->node->pos[1] = piece->offset[1];
                piece->node->pos[2] = piece->offset[2];
                break;
            case 12:
                piece->alpha *= ((Em *) obj->owner)->alpha;
                piece->node->pos[0] = obj->pos[0] + piece->offset[0];
                piece->node->pos[1] = obj->pos[1] + piece->offset[1];
                piece->node->pos[2] = obj->pos[2] + piece->offset[2];
                break;
            }
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
        piece++;
    }
}

/*
 * Runs the area fire's glow: the timer steps 1 or 2 at random, back to 1 past
 * 16 (the flicker FireBlast_Draw reads), and the slot goes into the sorted
 * draw table.
 */
void FireBlast_RunGlow(StageObj *obj)
{
    obj->timer += (Rand_Next(1) & 1) + 1;
    if (obj->timer > 0x10) {
        obj->timer = 1;
    }
    OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
}

/*
 * The end state: hidden, on to the free state; the flash (type 5) takes its
 * EmObj off the monsters' flash list (FlashObj_Remove). Frees the parts' draw
 * slots (those whose node is set), or the object's own slot for a type with
 * no parts.
 */
void FireBlast_End(StageObj *obj)
{
    u8 *piece = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    obj->visible = 0;
    if (obj->type == 5) {
        FlashObj_Remove((EmObj *)(piece + 0x16C));
    }
    count = FireBlast_PartCounts[obj->type];
    if (count == 0) {
        DrawSlot_Free(obj->slot);
    } else {
        for (i = 0; i < count; i++) {
            if (*(s32 *) (piece + 0x30) != 0) {
                DrawSlot_Free(*(s16 *) (piece + 4));
            }
            piece += 0x34;
        }
    }
}

/* The free state: frees the object. */
void FireBlast_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/* The area's fade, declared again by name here: the draw's flash case needs its load scheduled
 * before the scale[0] store, as the game's code has it, which a read through a pointer doesn't give. */
extern s16 gAreaFade;

/*
 * Returns from + (to - from) * frac, the difference taken as an s16: the mine
 * glow's alpha between two colours of FireBlast_MineRankColors.
 */
inline f32 Lerp(u32 from, u32 to, f32 frac)
{
    s16 diff = to - from;

    return from + diff * frac;
}

/*
 * Draws one part (node->part), or the glow's one slot, as a stage model
 * facing the camera with the part's opacity as its alpha. By type: the
 * blasts' fire from an animated 4x4 texture sheet and two other models, the
 * small blast likewise; the fires' flames orange from an 8x8 sheet; the glow
 * pulsing between 5 and 10% opaque with its timer; the flash bomb's lights;
 * the flash's models, drawn with flag 1 while the screen fade is in its first
 * two phases; the smoke's two models; the mine glow in its rank's colour,
 * cross-faded over 16 frames from the last rank's (FireBlast_MineRankColors).
 * Depth writes are off while it draws; nothing is drawn while the stage model
 * set is not loaded.
 */
void FireBlast_Draw(DrawSlot *node)
{
    u8 opacity;
    u8 green;
    ModelMat *mats;
    s16 id;
    u16 mode = 0;
    u8 red;
    Model *set = gStageModelSet;
    StageObj *obj = node->owner;
    FireBlastPart *piece = (FireBlastPart *) obj->buf + node->part;
    u16 order = 0;
    u8 blue;
    s16 diff;
    s16 fade;
    ColorRgba *color;
    ColorRgba *prevColor;
    f32 frac;
    f32 rot[3];
    f32 scale[3];
    f32 mtx[16];
    f32 uvMtx[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        red = 0xFF;
        green = 0xFF;
        blue = 0xFF;
        opacity = 255.0f * piece->alpha;
        switch (obj->type) {
        case 0:
        case 11:
            Gs_SetState(0x6C, 0);
            mode |= 2;
            scale[0] = obj->scale * piece->curveScale[0];
            scale[1] = obj->scale * piece->curveScale[1];
            scale[2] = obj->scale * piece->curveScale[2];
            switch (piece->look) {
            case 0:
            case 1:
            case 2:
                id = 0x4E;
                Mtx_Translation(uvMtx, 0.25f * (piece->cell & 3), 0.25f * (piece->cell / 4), 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 3:
                id = 0x50;
                break;
            case 4:
                id = 0x58;
                break;
            }
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 1:
            scale[0] = piece->size * piece->curveScale[0];
            id = 0x4E;
            mode |= 2;
            scale[1] = piece->size * piece->curveScale[1];
            scale[2] = piece->size * piece->curveScale[2];
            Mtx_Translation(uvMtx, 0.25f * (piece->cell & 3), 0.25f * (piece->cell >> 2), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 2:
            id = 0x15;
            scale[0] = obj->scale;
            scale[1] = obj->scale;
            scale[2] = obj->scale;
            Gs_SetState(0x6C, 0);
            mode |= 2;
            opacity = 255.0f * (0.05f + 0.025f * (1.0f + Math_Sin((6.2831855f * (360.0f * (obj->timer / 16.0f) - 90.0f)) / 360.0f)));
            break;
        case 3:
        case 6:
        case 12:
            id = 0x60;
            scale[0] = piece->curveScale[0];
            scale[1] = piece->curveScale[1];
            scale[2] = piece->curveScale[2];
            Gs_SetState(0x6C, 0);
            mode |= 2;
            Mtx_Translation(uvMtx, 0.0625f * (piece->cell & 7), 0.0625f * (piece->cell >> 3), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            red = 0xFF;
            green = 0x5F;
            blue = 0;
            break;
        case 4:
            scale[0] = piece->curveScale[0];
            scale[1] = piece->curveScale[1];
            scale[2] = piece->curveScale[2];
            switch (piece->look) {
            case 0:
            case 1:
                mode |= 2;
                id = 0x15;
                break;
            case 2:
                id = 0x23;
                break;
            case 3:
                id = 0x48;
                scale[0] *= 2.0f;
                scale[1] *= 2.0f;
                scale[2] *= 2.0f;
                break;
            }
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 5:
            scale[0] = piece->curveScale[0];
            fade = gAreaFade;
            scale[1] = piece->curveScale[1];
            scale[2] = piece->curveScale[2];
            id = node->part + 0x51;
            if (fade == 1 || fade == 2) {
                mode |= 1;
            } else if (piece->look < 2) {
                mode |= 2;
            }
            break;
        case 8:
            Gs_SetState(0x6C, 0);
            scale[0] = piece->curveScale[0] * (obj->scale * piece->size);
            scale[1] = piece->curveScale[1] * (obj->scale * piece->size);
            scale[2] = piece->curveScale[2] * (obj->scale * piece->size);
            if (!(piece->look & 1)) {
                id = 0x4D;
                Mtx_Translation(uvMtx, 0.0f, 0.25f, 0.0f);
            } else {
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                id = 0x4F;
            }
            Gs_SetState(0x19, (s32) uvMtx);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 9:
            scale[0] = 3.0f * obj->scale * piece->curveScale[0];
            scale[1] = 3.0f * obj->scale * piece->curveScale[1];
            scale[2] = 3.0f * obj->scale * piece->curveScale[2];
            Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            id = 0x22;
            mode |= 2;
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 10:
            scale[0] = piece->curveScale[0];
            scale[1] = piece->curveScale[1];
            scale[2] = piece->curveScale[2];
            id = FireBlast_MineGlowModels[piece->look];
            Gs_SetState(0x6C, 0);
            color = &FireBlast_MineRankColors[obj->var0];
            if (obj->var0 != 0 && obj->angle < 16) {
                prevColor = &FireBlast_MineRankColors[obj->var0 - 1];
                frac = obj->angle / 16.0f;
                diff = color->r - prevColor->r;
                red = prevColor->r + diff * frac;
                diff = color->g - prevColor->g;
                green = prevColor->g + diff * frac;
                diff = color->b - prevColor->b;
                blue = prevColor->b + diff * frac;
                opacity = piece->alpha * Lerp(prevColor->alpha, color->alpha, frac);
            } else {
                red = color->r;
                green = color->g;
                blue = color->b;
                opacity = piece->alpha * color->alpha;
            }
            break;
        }
        Gs_SetState(0x60, 0);
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Effect_DrawModel(&set->matSets[id], mtx, blue | ((green << 8) | ((opacity << 24) | (red << 16))), mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Creates a FireBlast of type k at pos with size scale and no owner, in any
 * area; types below 5 get a random angle. A bomb's blast (type 0) also makes
 * its smoke (type 8) of the same size; type 11 is made 0.7 times smaller.
 */
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = STAGEOBJ_KIND_FIRE_BLAST;
        obj->update = FireBlast_Update;
        obj->type = k;
        obj->owner = NULL;
        obj->scale = scale;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        if (k < 5) {
            obj->angle = Rand_Next(1);
        }
        if (k == 0) {
            FireBlast_CreateScaled(pos, 8, scale);
        }
        if (k == 0xB) {
            obj->scale *= 0.7f;
        }
    }
}

/*
 * Creates a FireBlast of type k at pos with no owner, in any area: the area
 * fire's glow (2) without buffer blocks at size 2, the others at size 1 (the
 * area fire's flames 3, the special mine point's glow 10).
 */
void FireBlast_CreateAt(f32 *pos, s16 k)
{
    StageObj *obj;

    if (k == 2) {
        obj = StageObj_Alloc(0);
    } else {
        obj = StageObj_Alloc(1);
    }
    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_FIRE_BLAST;
        obj->update = FireBlast_Update;
        obj->type = k;
        obj->owner = NULL;
        if (k == 2) {
            obj->scale = 2.0f;
        } else {
            obj->scale = 1.0f;
        }
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
    }
}

/*
 * Creates a FireBlast of type k at pos with size scale for owner, when owner
 * is in the current area; types below 5 get a random angle. Made for a thrown
 * flash bomb's burst (4) and its flash (5), and for EmKutku_MotionEffects'
 * flash (5).
 */
void FireBlast_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner)
{
    StageObj *obj;

    if (Obj_InCurrentArea(owner) & 0xFF) {
        if ((obj = StageObj_Alloc(1)) != NULL) {
            obj->kind = STAGEOBJ_KIND_FIRE_BLAST;
            obj->update = FireBlast_Update;
            obj->type = k;
            obj->owner = owner;
            obj->scale = scale;
            obj->pos[0] = pos[0];
            obj->pos[1] = pos[1];
            obj->pos[2] = pos[2];
            if (k < 5) {
                obj->angle = Rand_Next(1);
            }
        }
    }
}

/*
 * Creates a FireBlast of type k on em when em is in the current area, at size
 * 1. The spit-roast fire (6; em is the roasting hunter) sits 128 ahead of em
 * on its floor; the monster's flame (12) is placed by its run.
 */
void FireBlast_CreateOnEm(Em *em, s16 k)
{
    StageObj *obj;
    f32 offset[3];

    if (Obj_InCurrentArea(em) & 0xFF) {
        if ((obj = StageObj_Alloc(1)) != NULL) {
            obj->kind = STAGEOBJ_KIND_FIRE_BLAST;
            obj->update = FireBlast_Update;
            obj->type = k;
            obj->owner = em;
            obj->scale = 1.0f;
            if (k == 6) {
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 128.0f;
                Vec_RotateY(offset, (6.2831855f * em->rotY) / 65536.0f);
                obj->pos[0] = em->posX + offset[0];
                obj->pos[1] = em->floorY + offset[1];
                obj->pos[2] = em->posZ + offset[2];
            }
        }
    }
}

/* As FireBlast_CreateScaled, but type 11 keeps its size. */
void FireBlast_CreateScaledExact(f32 *pos, s16 k, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = STAGEOBJ_KIND_FIRE_BLAST;
        obj->update = FireBlast_Update;
        obj->type = k;
        obj->owner = NULL;
        obj->scale = scale;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        if (k < 5) {
            obj->angle = Rand_Next(1);
        }
        if (k == 0) {
            FireBlast_CreateScaled(pos, 8, scale);
        }
    }
}

/*
 * ElementBurst's update, run every frame: switches on obj->state to its set-
 * up, run, end and free states.
 */
void ElementBurst_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        ElementBurst_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        ElementBurst_Run(obj);
        break;
    case STAGEOBJ_END:
        ElementBurst_End(obj);
        break;
    case STAGEOBJ_FREE:
        ElementBurst_Free(obj);
        break;
    }
}

/*
 * The set-up state: visible, timer 0, no repeats made (var1); the rings at a
 * point (4) play their sound. Sets up the parts
 * (ElementBurst_PartCounts[type]), each at the object with the object's size,
 * a draw slot that ElementBurst_Draw draws and its start: fire (0) at
 * ElementBurst_FireDelays, its parts 0 and 2 spinning; poison and status (1,
 * 6) 5 frames apart, each 0.2 smaller; water (2) at ElementBurst_WaterDelays,
 * parts 0 and 1 at three quarters size; thunder, status 4 and ice (3, 7, 8) 5
 * frames late (ice's part 1 1.3 times larger); the rings (4, 5) at
 * ElementBurst_RingDelays; type 9 a single part. With var2 1 (a bowgun's
 * muzzle) water, thunder, status 4 and ice start at once.
 */
void ElementBurst_Setup(StageObj *obj)
{
    s16 i;
    ElementBurstPart *piece = (ElementBurstPart *) obj->buf;
    s16 count;

    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    count = ElementBurst_PartCounts[obj->type];
    switch (obj->type) {
    case 4:
        ElementBurst_PlaySound(obj);
        break;
    }
    for (i = 0; i < count; piece++, i++) {
        piece->slot = DrawSlot_Alloc();
        piece->kind = i;
        piece->scale = obj->scale;
        piece->frame = 0;
        switch (obj->type) {
        case 0:
            piece->angle = Rand_Next(1);
            piece->frame = ElementBurst_FireDelays[i] - 4;
            Vec_Copy(piece->pos, obj->pos);
            switch (piece->kind) {
            case 0:
                piece->spin = (Rand_Next(1) & 0x3FF) - 0x200;
                break;
            case 2:
                piece->spin = (Rand_Next(1) & 0x7FF) - 0x400;
                break;
            default:
                piece->spin = 0;
                break;
            }
            break;
        case 1:
        case 6:
            piece->angle = Rand_Next(1);
            piece->frame = i * -5 - 5;
            Vec_Copy(piece->pos, obj->pos);
            piece->scale *= 1.0f - 0.2f * i;
            piece->spin = (Rand_Next(1) & 0xFF) - 0x80;
            break;
        case 2:
            piece->angle = Rand_Next(1);
            if (obj->var2 != 1) {
                piece->frame = ElementBurst_WaterDelays[i] - 5;
            }
            Vec_Copy(piece->pos, obj->pos);
            switch (piece->kind) {
            case 0:
            case 1:
                piece->scale *= 0.75f;
                piece->spin = 0;
                break;
            case 2:
            default:
                piece->spin = 0;
                break;
            }
            break;
        case 3:
        case 7:
        case 8:
            if (obj->var2 != 1) {
                piece->frame -= 5;
            }
            piece->angle = Rand_Next(1);
            Vec_Copy(piece->pos, obj->pos);
            if (obj->type == 8 && piece->kind == 1) {
                piece->scale *= 1.3f;
            }
            break;
        case 4:
        case 5:
            piece->frame = ElementBurst_RingDelays[i];
            switch (i) {
            case 5:
            case 6:
                piece->angle = Rand_Next(1);
                break;
            default:
                piece->angle = 0;
                break;
            }
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 9:
            piece->kind = 1;
            piece->angle = Rand_Next(1);
            Vec_Copy(piece->pos, obj->pos);
            break;
        }
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = ElementBurst_Draw;
        } else {
            piece->node = NULL;
        }
    }
}

/*
 * Runs a frame. The life is ElementBurst_Limits[type], for the rings ten
 * frames more per repeat (var2); the rings on a monster (5) follow its bone
 * (ElementBurst_RoarBone) and end with the monster or when that gives -1; the
 * other types play their sound at frame 5. Each part waits out its delay,
 * then takes its scale, and for most types its opacity, from
 * ElementBurst_Curves by its frame (from ElementBurst_CurveIdx[type],
 * ElementBurst_Steps[type] curves a part), for ElementBurst_Durations[type]
 * frames or its own length from ElementBurst_DurLists; a finished ring part
 * repeats (ElementBurst_RepeatRing). Fire's parts 0 and 2 move 100 along the
 * yaw over their length, the poison and status puffs rise 60, those and the
 * rings stack toward the camera; each part goes into the sorted draw table.
 */
void ElementBurst_Run(StageObj *obj)
{
    ElementBurstPart *piece = (ElementBurstPart *) obj->buf;
    s16 i;
    Em *em = obj->owner;
    s16 count;
    s16 limit;
    s16 step;
    s16 dur;
    s16 curveIdx;
    s16 *durs;
    s16 bone;
    s32 curveAt;
    void *curve;
    f32 opacity;
    f32 camOffset[3];
    f32 move[3];

    count = ElementBurst_PartCounts[obj->type];
    limit = ElementBurst_Limits[obj->type];
    step = ElementBurst_Steps[obj->type];
    curveIdx = ElementBurst_CurveIdx[obj->type];
    dur = ElementBurst_Durations[obj->type];
    durs = ElementBurst_DurLists[obj->type];
    switch (obj->type) {
    case 4:
        limit += (s16) (obj->var2 * 10);
        break;
    case 5:
        if (em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        bone = ElementBurst_RoarBone(obj);
        if (bone < 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        Em_BonePos(em, bone, obj->pos);
        limit += (s16) (obj->var2 * 10);
        break;
    default:
        if (obj->timer == 5) {
            ElementBurst_PlaySound(obj);
        }
        break;
    }
    if (++obj->timer > limit) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < count; i++, piece++) {
        if (durs == NULL) {
            curveIdx = ElementBurst_CurveIdx[obj->type];
        } else {
            dur = durs[i];
        }
        camOffset[0] = 0.0f;
        camOffset[1] = 0.0f;
        camOffset[2] = 0.0f;
        if (++piece->frame <= 0) {
            curveIdx += step;
            continue;
        }
        if (piece->frame > dur) {
            if (obj->type == 4 || obj->type == 5) {
                if ((s16) ElementBurst_RepeatRing(obj, piece) == 0) {
                    curveIdx += step;
                    continue;
                }
            } else {
                curveIdx += step;
                continue;
            }
        }
        switch (obj->type) {
        case 0:
            curveAt = curveIdx;
            curve = ElementBurst_Curves[curveAt];
            Key_LerpVec3(curve, piece->sc, piece->frame);
            curveIdx += 2;
            Key_LerpFloat(ElementBurst_Curves[(s16) (curveAt + 1)], &opacity, piece->frame);
            piece->alpha = 255.0f * opacity;
            switch (piece->kind) {
            case 0:
            case 2:
                move[0] = 0.0f;
                move[1] = 0.0f;
                move[2] = 100.0f / dur;
                Vec_RotateY(move, (6.2831855f * obj->angle) / 65536.0f);
                piece->pos[0] += move[0];
                piece->pos[1] += move[1];
                piece->pos[2] += move[2];
                break;
            }
            piece->angle += piece->spin;
            break;
        case 1:
        case 6:
            curve = ElementBurst_Curves[curveIdx++];
            Key_LerpVec3(curve, piece->sc, piece->frame);
            piece->angle += piece->spin;
            piece->pos[1] += 60.0f / dur;
            camOffset[2] = 10.0f * i;
            Vec_RotateSelf(camOffset, gCamMatrix);
            break;
        case 2:
            curve = ElementBurst_Curves[curveIdx++];
            Key_LerpVec3(curve, piece->sc, piece->frame);
            piece->angle += piece->spin;
            break;
        case 3:
        case 7:
        case 8:
        case 9:
            curveAt = curveIdx;
            curve = ElementBurst_Curves[curveAt];
            Key_LerpVec3(curve, piece->sc, piece->frame);
            curveIdx += 2;
            Key_LerpFloat(ElementBurst_Curves[(s16) (curveAt + 1)], &opacity, piece->frame);
            piece->alpha = 255.0f * opacity;
            break;
        case 5:
            Vec_Copy(piece->pos, obj->pos);
        case 4:
            curveAt = curveIdx;
            curve = ElementBurst_Curves[curveAt];
            Key_LerpVec3(curve, piece->sc, piece->frame);
            curveIdx += 2;
            Key_LerpFloat(ElementBurst_Curves[(s16) (curveAt + 1)], &opacity, piece->frame);
            piece->alpha = 255.0f * opacity;
            camOffset[0] = 0.0f;
            camOffset[1] = 0.0f;
            camOffset[2] = 20.0f - 5.0f * i;
            Vec_RotateSelf(camOffset, gCamMatrix);
            break;
        }
        if (piece->node != NULL) {
            piece->node->pos[0] = camOffset[0] + piece->pos[0];
            piece->node->pos[1] = camOffset[1] + piece->pos[1];
            piece->node->pos[2] = camOffset[2] + piece->pos[2];
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
    }
}

/* The end state: on to the free state; frees the parts' draw slots. */
void ElementBurst_End(StageObj *obj)
{
    s16 count;
    s16 i;
    ElementBurstPart *piece = (ElementBurstPart *) obj->buf;

    obj->state++;
    count = ElementBurst_PartCounts[obj->type];
    for (i = 0; i < count; i++) {
        if (piece->node != NULL) {
            DrawSlot_Free(piece->slot);
        }
        piece++;
    }
}

/* The free state: frees the object. */
void ElementBurst_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

/*
 * Draws one part (node->part) as a stage model facing the camera, scaled by
 * the part's size times its curve scale and turned by its angle, depth writes
 * off. By type and part kind: fire an animated 4x4 sheet and two flares;
 * poison and status model 72 coloured by ElementBurst_PoisonColorKeys or
 * ElementBurst_StatusColorKeys by frame; water two models picked by the frame
 * and a ring coloured by ElementBurst_WaterRingColorKeys; thunder, status 4,
 * ice and type 9 a flare and a scrolling streak; the rings four models (type
 * 5 skips part 0). The type is read into a local once for the switch and its
 * tests, while the poison case reads obj->type again: the compiler then loads
 * it as the game's code does.
 */
void ElementBurst_Draw(DrawSlot *node)
{
    f32 mtx[16];
    u32 type;
    f32 uvMtx[16];
    f32 scale[3];
    u32 color;
    StageObj *obj = node->owner;
    Model *set = gStageModelSet;
    ElementBurstPart *piece = (ElementBurstPart *) obj->buf + node->part;
    f32 rot[3];
    ModelMat *mats;
    ModelMatSet *model;
    u16 mode = 0;
    s32 order;
    s32 cell;

    if (set != NULL && set->active != 0) {
        mats = set->mats;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        type = obj->type;
        switch (type) {
        case 0:
            switch (piece->kind) {
            case 0:
                model = &set->matSets[54];
                mode |= 2;
                cell = piece->frame - 1;
                Mtx_Translation(uvMtx, 0.25f * (cell & 3), 0.25f * (cell >> 2), 0.0f);
                break;
            case 1:
                mode |= 2;
                model = &set->matSets[66];
                Mtx_Translation(uvMtx, 0.0f, 0.75f, 0.0f);
                break;
            case 2:
                model = &set->matSets[94];
                Mtx_Translation(uvMtx, 0.0f, 0.75f, 0.0f);
                break;
            }
            Gs_SetState(0x19, (s32) uvMtx);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            color = (piece->alpha << 24) | 0xFFFFFF;
            break;
        case 1:
        case 6:
            model = &set->matSets[72];
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            Color_EvalKeys((s32 *) (obj->type == 1 ? ElementBurst_PoisonColorKeys : ElementBurst_StatusColorKeys), piece->frame, &color);
            break;
        case 2:
            switch (piece->kind) {
            case 0:
                model = &set->matSets[piece->frame];
                Mtx_Translation(uvMtx, 0.5f, 0.124f, 0.0f);
                color = -1;
                break;
            case 1:
                model = set->matSets + piece->frame + 27;
                Mtx_Translation(uvMtx, 0.5f, 0.004f, 0.0f);
                color = -1;
                break;
            case 2:
            default:
                model = &set->matSets[94];
                Mtx_Translation(uvMtx, 0.75f, 0.75f, 0.0f);
                Color_EvalKeys((s32 *) ElementBurst_WaterRingColorKeys, piece->frame, &color);
                break;
            }
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x19, (s32) uvMtx);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            break;
        case 3:
        case 7:
        case 8:
        case 9:
            if (type == 8) {
                mode |= 8;
            } else {
                mode |= 2;
            }
            if (piece->kind == 0) {
                model = &set->matSets[66];
                if (type == 3 || type == 8 || type == 9) {
                    Mtx_Translation(uvMtx, 0.25f, 0.75f, 0.0f);
                } else {
                    Mtx_Translation(uvMtx, 0.0f, 0.75f, 0.0f);
                }
            } else {
                model = &set->matSets[75];
                if (type == 3 || type == 8 || type == 9) {
                    Mtx_Translation(uvMtx, 0.0f, 0.046875f * piece->frame, 0.0f);
                } else {
                    Mtx_Translation(uvMtx, 0.0625f, 0.5f + 0.046875f * piece->frame, 0.0f);
                }
            }
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x19, (s32) uvMtx);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            color = (piece->alpha << 24) | 0xFFFFFF;
            break;
        case 4:
        case 5:
            switch (piece->kind) {
            case 0:
                if (type == 5) {
                    return;
                }
                model = &set->matSets[0];
                mode |= 2;
                Mtx_Translation(uvMtx, 0.125f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
                break;
            case 1:
                model = &set->matSets[64];
                break;
            case 2:
            case 3:
            case 4:
                model = &set->matSets[88];
                break;
            case 5:
            case 6:
                model = &set->matSets[35];
                break;
            }
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            order = 3;
            color = (piece->alpha << 24) | 0xFFFFFF;
            break;
        }
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Effect_DrawModel(model, mtx, color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Repeats a finished ring part (types 4, 5) while fewer than var2 repeats
 * were made: parts of kinds 3 and 4 wait again (14 frames in the first round,
 * 6 after); a part of kind 6 restarts at frame 1 with a new angle and counts
 * a repeat (var1). Returns 1 when the part runs this frame, 0 when it waits
 * or no repeats are left.
 */
s32 ElementBurst_RepeatRing(StageObj *obj, ElementBurstPart *piece)
{
    if (obj->var1 >= obj->var2) {
        return 0;
    }
    switch (piece->kind) {
    case 3:
    case 4:
        if (obj->var1 == 0) {
            piece->frame = -0xE;
        } else {
            piece->frame = -6;
        }
        break;
    case 6:
        piece->angle = Rand_Next(1);
        piece->frame = 1;
        obj->var1++;
        return 1;
    }
    return 0;
}

/*
 * Returns the bone the rings on a monster (type 5) follow, by the monster's
 * kind, once its motion (likely its roar; Blangonga's are BLANG_MOT_ROAR and
 * _ROAR_B) has reached the kind's frame: 0x22 for the Rathalos, Gravios,
 * Diablos and Khezu kinds, 0x33 Fatalis, 0x25 Lao-Shan Lung, 0x23 Yian
 * Garuga, 0x13 Blangonga, Rajang and Congalala, 0x27 Kushala Daora, Lunastra
 * and Teostra. Returns -1 in other motions and for other kinds.
 */
s32 ElementBurst_RoarBone(StageObj *obj)
{
    Em *em = obj->owner;

    switch (em->type) {
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
        if (!((em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x41B && EmMotion_ReachedFrame(em, 0, 160.0f)))) {
            return -1;
        }
        return 0x22;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_BASARIOS:
        if (!((em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x452 && EmMotion_ReachedFrame(em, 0, 110.0f)))) {
            return -1;
        }
        return 0x22;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
        if (!((em->motionId[0] == 0x457 && EmMotion_ReachedFrame(em, 0, 112.0f)) ||
            (em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x41B && EmMotion_ReachedFrame(em, 0, 160.0f)))) {
            return -1;
        }
        return 0x22;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (!(em->motionId[0] == 0x408 && EmMotion_ReachedFrame(em, 0, 66.0f))) {
            return -1;
        }
        return 0x33;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        if (!((em->motionId[0] == 0x451 && EmMotion_ReachedFrame(em, 0, 66.0f)) ||
            (em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 78.0f)))) {
            return -1;
        }
        return 0x22;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        if (!(em->motionId[0] == 0x3EF && EmMotion_ReachedFrame(em, 0, 4.0f))) {
            return -1;
        }
        return 0x25;
    case EM_KIND_YIAN_GARUGA:
        if (!(em->motionId[0] == 0x410 && EmMotion_ReachedFrame(em, 0, 60.0f))) {
            return -1;
        }
        return 0x23;
    case EM_KIND_BLANGONGA:
    case EM_KIND_RAJANG:
        if (!((em->motionId[0] == 0x41C && EmMotion_ReachedFrame(em, 0, 38.0f)) ||
            (em->motionId[0] == 0x423 && EmMotion_ReachedFrame(em, 0, 64.0f)))) {
            return -1;
        }
        return 0x13;
    case EM_KIND_CONGALALA:
        if (!(em->motionId[0] == 0x43E && EmMotion_ReachedFrame(em, 0, 100.0f))) {
            return -1;
        }
        return 0x13;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        if (!(em->motionId[0] == 0x408 && EmMotion_ReachedFrame(em, 0, 62.0f))) {
            return -1;
        }
        return 0x27;
    }
    return -1;
}

/*
 * Plays the type's sound at the object from the common bank: fire's, water's,
 * thunder's (also status 4's), ice's, and type 4's unless silenced (angle 1);
 * water, thunder, status 4 and ice stay silent for a muzzle (var2 1). Poison,
 * status and type 5 have none.
 */
void ElementBurst_PlaySound(StageObj *obj)
{
    switch (obj->type) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x5E, 0, obj->pos, 1, 0);
        break;
    case 2:
        if (obj->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x5F, 0, obj->pos, 1, 0);
        }
        break;
    case 3:
    case 7:
        if (obj->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x61, 0, obj->pos, 1, 0);
        }
        break;
    case 8:
        if (obj->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x62, 0, obj->pos, 1, 0);
        }
        break;
    case 4:
        if (obj->angle == 0) {
            Sound_PlaySeAtPosChecked(1, 0x75, 0, obj->pos, 6, 0);
        }
        break;
    case 1:
    case 5:
    case 6:
        break;
    }
}

/*
 * Creates an ElementBurst of type k at pos with the yaw (the hit's angle),
 * owner and size scale, when owner is NULL or in the current area; no
 * repeats. ObjAttach_HitEffects makes one where a hunter's elemental or
 * status hit lands.
 */
void ElementBurst_Create(f32 *pos, s32 k, u16 yaw, void *owner, f32 scale)
{
    StageObj *obj;

    if (owner == NULL || (Obj_InCurrentArea(owner) & 0xFF)) {
        if ((obj = StageObj_Alloc(1)) != NULL) {
            obj->kind = STAGEOBJ_KIND_ELEMENT_BURST;
            obj->update = ElementBurst_Update;
            obj->type = k;
            Vec_Copy(obj->pos, pos);
            obj->angle = yaw;
            obj->var2 = 0;
            obj->owner = owner;
            obj->scale = scale;
        }
    }
}

/*
 * Creates an ElementBurst of type k at pos with the given repeats (var2),
 * size scale, no owner and yaw 0, in any area. Returns it, or NULL when none
 * is free.
 */
StageObj *ElementBurst_CreateAt(f32 *pos, s32 k, s32 repeats, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) == NULL) {
        return NULL;
    }
    obj->kind = STAGEOBJ_KIND_ELEMENT_BURST;
    obj->update = ElementBurst_Update;
    obj->type = k;
    Vec_Copy(obj->pos, pos);
    obj->var2 = repeats;
    obj->owner = NULL;
    obj->angle = 0;
    obj->scale = scale;
    return obj;
}

/* As ElementBurst_CreateAt, with angle 1, which silences type 4. */
void ElementBurst_CreateAtSilent(f32 *pos, s32 k, s32 repeats, f32 scale)
{
    StageObj *obj;

    obj = ElementBurst_CreateAt(pos, k, repeats, scale);
    if (obj != NULL) {
        obj->angle = 1;
    }
}

/*
 * Creates an ElementBurst of type k on monster em, when em is in the current
 * area, with the given repeats (var2) and size scale; its run places it (type
 * 5 at ElementBurst_RoarBone's bone).
 */
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 repeats, f32 scale)
{
    StageObj *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_ELEMENT_BURST;
            obj->update = ElementBurst_Update;
            obj->type = k;
            obj->var2 = repeats;
            obj->owner = em;
            obj->scale = scale;
        }
    }
}

/*
 * Creates an ElementBurst of type k at pos with the yaw, size scale, var2
 * quiet and no owner, in any area. Bowgun_FireShot's muzzle effects pass
 * quiet 1: their parts start at once, silent.
 */
void ElementBurst_CreateAtYaw(f32 *pos, u8 k, u8 quiet, u16 yaw, f32 scale)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = STAGEOBJ_KIND_ELEMENT_BURST;
        obj->update = ElementBurst_Update;
        obj->type = k;
        Vec_Copy(obj->pos, pos);
        obj->angle = yaw;
        obj->var2 = quiet;
        obj->owner = NULL;
        obj->scale = scale;
    }
}

/*
 * HitSplash's update, run every frame: switches on obj->state to its set-up,
 * run, end and free states.
 */
void HitSplash_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        HitSplash_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        HitSplash_Run(obj);
        break;
    case STAGEOBJ_END:
        HitSplash_End(obj);
        break;
    case STAGEOBJ_FREE:
        HitSplash_Free(obj);
        break;
    }
}

/*
 * The set-up state: visible, timer 0. The flashes and the spark (2, 5, 6)
 * play their sound at the object, type 8 at its bone when it has one; types 3
 * and 4 set up no parts. Sets up the parts (HitSplash_PartCounts[type]) at
 * the object, each with a draw slot that HitSplash_Draw draws: the spray's
 * (0) start 10 to 17 frames late (at once for var0 0x20) and fall 0.8 a
 * frame; the flash's (2) a frame apart; the strong flash's (5) first three 60
 * degrees apart (plus a random turn) round its angle, the rest at random
 * angles; the spark's (6) at once; those of types 8, 9, 12, 13 and 15 of
 * random sizes (0.5 to 1 times the object's) at their own delays (13: var1
 * frames apart); type 14's wait var1 frames.
 */
void HitSplash_Setup(StageObj *obj)
{
    HitSplashPart *piece = (HitSplashPart *) obj->buf;
    s16 count;
    s16 i;
    f32 pos[3];

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    count = HitSplash_PartCounts[obj->type];
    switch (obj->type) {
    case 2:
    case 5:
    case 6:
        HitSplash_PlaySound(obj, obj->pos);
        break;
    case 8:
        if (obj->var2 != 0xFF) {
            Em_GetBonePos(obj->owner, obj->var2, pos);
            HitSplash_PlaySound(obj, pos);
        }
        break;
    case 3:
    case 4:
        return;
    }
    for (i = 0; i < count; piece++, i++) {
        switch (obj->type) {
        case 0:
            piece->idx = i;
            if (!(obj->var0 & 0x20)) {
                piece->delay = -10 - (Rand_Next(1) & 7);
            } else {
                piece->delay = 0;
            }
            piece->scale = obj->scale;
            piece->fall = -0.8f;
            break;
        case 2:
            piece->delay = -i;
            piece->idx = 0;
            piece->alpha = 0xFF;
            piece->scale = obj->scale;
            piece->angle = Rand_Next(1);
            break;
        case 5:
            if (i < 3) {
                piece->idx = 0;
                piece->delay = -i;
                piece->scale = obj->scale;
                piece->angle = obj->angle + (i * 0x2AAA + Rand_Next(1) / 3);
            } else {
                piece->idx = 1;
                piece->delay = 0;
                piece->scale = obj->scale;
                piece->angle = Rand_Next(1);
            }
            piece->alpha = 0xFF;
            break;
        case 6:
            piece->delay = 0;
            piece->idx = i;
            if (i == 0) {
                piece->alpha = 0xFF;
            }
            piece->scale = obj->scale;
            break;
        case 8:
            piece->idx = i;
            piece->delay = i * -4 - 10;
            piece->scale = obj->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            piece->cell = 0;
            break;
        case 9:
            piece->idx = Rand_Next(1) & 7;
            piece->delay = i * -12 - 10;
            piece->scale = obj->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 12:
            piece->idx = Rand_Next(1) & 7;
            piece->delay = i * -10;
            piece->scale = obj->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 13:
            piece->idx = Rand_Next(1) & 7;
            piece->delay = -obj->var1 * i;
            piece->scale = obj->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 14:
            piece->idx = i;
            piece->cell = Rand_Next(1) & 3;
            piece->angle = Rand_Next(1);
            piece->delay = -obj->var1;
            piece->scale = obj->scale;
            piece->alpha = 0xFF;
            break;
        case 15:
            piece->idx = i;
            piece->delay = 0;
            piece->scale = obj->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            piece->cell = 0;
            break;
        }
        piece->pos[0] = obj->pos[0];
        piece->pos[1] = obj->pos[1];
        piece->pos[2] = obj->pos[2];
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = HitSplash_Draw;
        } else {
            piece->node = NULL;
        }
    }
}

/*
 * Runs a frame. The life is HitSplash_Lives[type] (types 13 and 14: the
 * angle; 14 with a negative one never ends). When the object follows a bone
 * (var2) and its owner is gone, the spray and types 8 and 9 hide their parts,
 * types 3 and 4 end. While the spray's first part lives, at each step of its
 * table (HitSplash_SpraySteps, _SprayStepsStrong or _SprayStepsAtOnce by
 * var0) it bursts (HitSpark_SpawnForOwner) at the bone's point, sized by the
 * step and its target's HitSplash_TargetScales row, the first step sounding
 * for a monster. Each part waits out its delay; on starting, one following a
 * bone takes the bone's point (the spray's a random flight turned by the
 * angle). It takes its scale and opacity from HitSplash_Curves by its frame
 * (the flashes pick new models and angles each frame, types 8 and 15 step
 * their cell at HitSplash_CellFrames) and moves: the spray's fly and fall,
 * the flashes and spark stack toward the camera, the others rise (some
 * spinning by HitSplash_SpinSpeed, the last parts smaller and fainter); type
 * 13 restarts its parts. Each part goes into the sorted draw table.
 */
void HitSplash_Run(StageObj *obj)
{
    s16 k;
    f32 off[3];
    f32 pos[3];
    f32 m[16];
    HitSplashPart *piece;
    HitSplashStep *steps;
    s16 count;
    s16 life;
    f32 opacity;
    s16 end;
    s16 stride;
    s16 i;
    s16 stepFrame;
    s16 j;
    s16 found;
    s32 rowOrCurve;
    s32 modeOrCurve;
    s32 listIdx;
    void *curve;
    f32 burstSize;
    f32 x;
    f32 y;

    piece = (HitSplashPart *) obj->buf;
    count = HitSplash_PartCounts[obj->type];
    life = HitSplash_Lives[obj->type];
    k = HitSplash_CurveIdx[obj->type];
    end = HitSplash_Ends[obj->type];
    stride = HitSplash_Strides[obj->type];
    switch (obj->type) {
    case 13:
        life = obj->angle;
        break;
    case 14:
        life = obj->angle;
        if (life < 0) {
            life = 1000;
            obj->timer = 0;
            end = life;
        } else {
            end = life;
        }
        break;
    }
    if (++obj->timer > life) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    if (obj->var2 != 0xFF
        && (((Em *) obj->owner)->taskPhase == 3 || ((Em *) obj->owner)->active == 0)) {
        switch (obj->type) {
        case 0:
        case 8:
        case 9:
            for (i = 0; i < count; i++) {
                piece[i].idx = 0xFF;
            }
            break;
        case 3:
        case 4:
            obj->state++;
            obj->visible = 0;
            return;
        }
    }
    if (obj->type == 0 && piece->idx != 0xFF) {
        if ((obj->var0 & 0x20) == 0) {
            stepFrame = obj->timer - 10;
        } else {
            stepFrame = obj->timer;
        }
        modeOrCurve = obj->var0 & 0xF0;
        switch (modeOrCurve) {
        default:
            steps = HitSplash_SpraySteps;
            break;
        case 0x10:
            steps = HitSplash_SprayStepsStrong;
            break;
        case 0x20:
            steps = HitSplash_SprayStepsAtOnce;
            break;
        }
        j = 0;
        found = 0;
        for (; steps[j].frame != -1; j++) {
            if (stepFrame == steps[j].frame) {
                found = 1;
                break;
            }
        }
        if (found == 1) {
            if (obj->var2 != 0xFF) {
                Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(obj->owner, obj->var2)));
                Vec_Rotate(off, obj->pos, m);
                Mtx_GetTrans(pos, m);
                pos[0] += off[0];
                pos[1] += off[1];
                pos[2] += off[2];
                if (j == 0 && ((Em *) obj->owner)->isEm == 1 && (obj->var0 & 0xF0) != 0x20) {
                    HitSplash_PlaySound(obj, pos);
                }
                burstSize = 0.1f * steps[j].size;
                rowOrCurve = obj->var0 & 0xF;
                burstSize *= obj->scale * HitSplash_TargetScales[rowOrCurve];
                if (rowOrCurve == 0) {
                    burstSize *= Effect_Scale();
                }
                switch (obj->var0 & 0xF0) {
                case 0:
                    HitSpark_SpawnForOwner(obj->owner, (u16) obj->angle, 3, 2, pos, burstSize);
                    break;
                case 0x20:
                    HitSpark_SpawnForOwner(obj->owner, (u16) obj->angle, 3, 0, pos, burstSize);
                    break;
                case 0x10:
                    HitSpark_SpawnForOwner(obj->owner, (u16) obj->angle, 3, 3, pos, burstSize);
                    break;
                }
            } else {
                Vec_Copy(pos, obj->pos);
            }
        }
    }
    for (i = 0; i < count; i++) {
        switch (obj->type) {
        case 0:
        case 8:
        case 9:
        case 12:
        case 13:
        case 15:
            k = HitSplash_CurveIdx[obj->type];
            break;
        case 2:
            k = HitSplash_CurveIdx[obj->type];
            if (i == 2) {
                end = 4;
                k += stride;
            }
            break;
        case 5:
            if (piece->idx == 0) {
                end = 5;
                k = HitSplash_CurveIdx[obj->type];
            } else {
                end = 5;
                k = stride + HitSplash_CurveIdx[obj->type];
            }
            break;
        case 6:
            if (i != 0) {
                stride = 2;
                end = 5;
            }
            break;
        case 14:
            if (obj->angle < 0) {
                piece->delay = 0;
            }
            break;
        }
        if (++piece->delay <= 0) {
            piece++;
            k += stride;
            continue;
        }
        if (piece->delay == 1 && obj->type != 2 && obj->type != 5 && obj->type != 6 && obj->type != 14
            && obj->type != 15) {
            if (obj->var2 != 0xFF) {
                if (piece->idx == 0xFF) {
                    piece++;
                    continue;
                }
                Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(obj->owner, obj->var2)));
                Vec_Rotate(off, obj->pos, m);
                Mtx_GetTrans(pos, m);
                if (obj->type == 0) {
                    if (piece->node != NULL) {
                        piece->node->pos[0] = pos[0] + off[0];
                        piece->node->pos[1] = pos[1] + off[1];
                        piece->node->pos[2] = pos[2] + off[2];
                    }
                    off[0] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    off[1] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    off[2] = 30.0f + 0.005f * (Rand_Next(1) & 0x3FF);
                    x = off[0];
                    y = off[1];
                    Effect_ScaleVec(off);
                    off[0] = x;
                    off[1] = y;
                    piece->cell = Rand_Next(1) & 3;
                    piece->angle = Rand_Next(1);
                    Vec_Copy(piece->pos, off);
                    Vec_RotateY(piece->pos, (6.2831855f * obj->angle) / 65536.0f);
                } else if (obj->type == 8 || obj->type == 9) {
                    piece->pos[0] = pos[0] + off[0];
                    piece->pos[1] = pos[1] + off[1];
                    piece->pos[2] = pos[2] + off[2];
                    if (obj->type == 8) {
                        off[0] = 0.04f * ((Rand_Next(1) & 0x3FF) - 0x200);
                        off[1] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                        off[2] = 3.0f;
                        piece->pos[0] += off[0];
                        piece->pos[1] += off[1];
                        piece->pos[2] += off[2];
                    }
                } else {
                    piece->pos[0] = pos[0] + off[0] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    piece->pos[1] = 10.0f + (pos[1] + off[1] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200));
                    piece->pos[2] = pos[2] + off[2] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200);
                }
            }
        } else if (piece->delay > end) {
            if (obj->type == 13) {
                piece->delay = -obj->var1 * 9 + 60;
                Vec_Copy(piece->pos, obj->pos);
            }
            k += stride;
            piece++;
            continue;
        }
        switch (obj->type) {
        case 0:
        case 9:
        case 12:
        case 13:
            modeOrCurve = k;
            curve = (void *) HitSplash_Curves[modeOrCurve];
            Key_LerpVec3(curve, piece->sc, piece->delay);
            k += 2;
            Key_LerpFloat((void *) HitSplash_Curves[(s16) (modeOrCurve + 1)], &opacity, piece->delay);
            piece->alpha = 255.0f * opacity;
            break;
        case 2:
            curve = (void *) HitSplash_Curves[k++];
            Key_LerpVec3(curve, piece->sc, piece->delay);
            if (i == 2) {
                piece->angle = Rand_Next(1);
                piece->idx = Rand_Next(1) % 3;
            }
            break;
        case 5:
            curve = (void *) HitSplash_Curves[k++];
            Key_LerpVec3(curve, piece->sc, piece->delay);
            if (i >= 3) {
                piece->idx = Rand_Next(1) % 3 + 1;
                piece->angle = Rand_Next(1);
            }
            break;
        case 6:
            curve = (void *) HitSplash_Curves[k++];
            Key_LerpVec3(curve, piece->sc, piece->delay);
            if (i == 0) {
                piece->angle = Rand_Next(1);
                piece->idx = Rand_Next(1) % 3;
            } else {
                curve = (void *) HitSplash_Curves[k++];
                Key_LerpFloat(curve, &opacity, piece->delay);
                piece->alpha = 255.0f * opacity;
            }
            break;
        case 14:
            break;
        default:
            rowOrCurve = k;
            curve = (void *) HitSplash_Curves[rowOrCurve];
            Key_LerpVec3(curve, piece->sc, piece->delay);
            k += 2;
            Key_LerpFloat((void *) HitSplash_Curves[(s16) (rowOrCurve + 1)], &opacity, piece->delay);
            piece->alpha = 255.0f * opacity;
            for (listIdx = 0; HitSplash_CellFrames[listIdx] != -1; listIdx++) {
                if (piece->delay == HitSplash_CellFrames[listIdx]) {
                    piece->cell++;
                    break;
                }
            }
            break;
        }
        if (piece->node != NULL) {
            switch (obj->type) {
            case 0:
                piece->pos[1] += piece->fall;
                piece->node->pos[0] += piece->pos[0];
                piece->node->pos[1] += piece->pos[1];
                piece->node->pos[2] += piece->pos[2];
                break;
            case 2:
            case 5:
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = i * 5 + 15;
                Vec_RotateSelf(off, gCamMatrix);
                piece->node->pos[0] = piece->pos[0] + off[0];
                piece->node->pos[1] = piece->pos[1] + off[1];
                piece->node->pos[2] = piece->pos[2] + off[2];
                break;
            case 6:
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = i * 5 + 15;
                Vec_RotateSelf(off, gCamMatrix);
                piece->node->pos[0] = piece->pos[0] + off[0];
                piece->node->pos[1] = piece->pos[1] + off[1];
                piece->node->pos[2] = piece->pos[2] + off[2];
                break;
            case 8:
            case 15:
                switch (i) {
                case 6:
                    piece->sc[0] *= 0.8f;
                    piece->sc[1] *= 0.8f;
                    piece->sc[2] *= 0.8f;
                    piece->alpha = 0.8f * piece->alpha;
                    break;
                case 7:
                    piece->sc[0] *= 0.6f;
                    piece->sc[1] *= 0.6f;
                    piece->sc[2] *= 0.6f;
                    piece->alpha = 0.6f * piece->alpha;
                    break;
                case 8:
                    piece->sc[0] *= 0.4f;
                    piece->sc[1] *= 0.4f;
                    piece->sc[2] *= 0.4f;
                    piece->alpha = 0.4f * piece->alpha;
                    break;
                }
                piece->pos[1] += 3.5f + 0.003f * (Rand_Next(1) & 0x3FF);
                piece->node->pos[0] = piece->pos[0];
                piece->node->pos[1] = piece->pos[1];
                piece->node->pos[2] = piece->pos[2];
                break;
            case 9:
                switch (i) {
                case 4:
                    piece->alpha = 0.8f * piece->alpha;
                    break;
                case 5:
                    piece->alpha = 0.6f * piece->alpha;
                    break;
                case 6:
                    piece->alpha = 0.4f * piece->alpha;
                    break;
                }
                piece->angle += (u16) HitSplash_SpinSpeed(piece->idx);
                piece->pos[1] += 2.0f + 0.001f * (Rand_Next(1) & 0x3FF);
                piece->node->pos[0] = piece->pos[0];
                piece->node->pos[1] = piece->pos[1];
                piece->node->pos[2] = piece->pos[2];
                break;
            case 12:
            case 13:
                piece->angle += (u16) HitSplash_SpinSpeed(piece->idx);
                if (obj->type == 12) {
                    piece->alpha = 0.1f * piece->alpha;
                }
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = -(i * 5);
                Vec_RotateSelf(off, gCamMatrix);
                piece->pos[1] += 3.0f + 0.001f * (Rand_Next(1) & 0x3FF);
                piece->node->pos[0] = piece->pos[0] + off[0];
                piece->node->pos[1] = piece->pos[1] + off[1];
                piece->node->pos[2] = piece->pos[2] + off[2];
                break;
            case 14:
                Vec_Copy(piece->node->pos, obj->pos);
                break;
            }
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
        piece++;
    }
}

/* The end state: on to the free state; frees the parts' draw slots. */
void HitSplash_End(StageObj *obj)
{
    s16 count;
    s16 i;
    HitSplashPart *piece = (HitSplashPart *) obj->buf;

    obj->state++;
    count = HitSplash_PartCounts[obj->type];
    for (i = 0; i < count; i++) {
        if (piece->node != NULL) {
            DrawSlot_Free(piece->slot);
        }
        piece++;
    }
}

/* The free state: frees the object. */
void HitSplash_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

extern u8 gEffectColor0Green;
extern u8 gEffectColor0Blue;
/*
 * Draws one part (node->part) as a stage model facing the camera (type 14
 * lying flat instead) with the part's opacity, depth writes off. The spray:
 * model 97 from a 2x2 sheet in its target's row of gEffectColors
 * (Effect_OwnerColorRow of ownerIndex, the monster's kind), darkening to half
 * over frames 8 to 16. The flashes (2, 5): model 0 or one of three others,
 * white when var1 is 1, else in the target's colour; the spark two models;
 * types 8 and 15 model 78 from a 4x4 sheet; 9, 12 and 13 models 77 and 79
 * (12: 74) at their sizes (the test for type 10 there never holds); type 14
 * model 97 in gEffectColors' first colour.
 */
void HitSplash_Draw(DrawSlot *node)
{
    StageObj *obj = node->owner;
    s16 colorRow;
    HitSplashPart *piece = (HitSplashPart *) obj->buf + node->part;
    u16 mode = 0;
    ModelMatSet *model;
    ModelMat *mats;
    u8 red;
    u8 green;
    u8 blue;
    Model *set = gStageModelSet;
    f32 frac;
    f32 rot[3];
    f32 scale[3];
    f32 mtx[16];
    f32 uvMtx[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        red = 0xFF;
        green = 0xFF;
        blue = 0xFF;
        switch (obj->type) {
        case 0:
            Gs_SetState(0x6C, 0);
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            scale[0] = piece->sc[0] * (piece->scale * HitSplash_TargetScales[obj->var0 & 0xF]);
            scale[1] = piece->sc[1] * (piece->scale * HitSplash_TargetScales[obj->var0 & 0xF]);
            scale[2] = piece->sc[2] * (piece->scale * HitSplash_TargetScales[obj->var0 & 0xF]);
            if ((obj->var0 & 0xF) == 0) {
                Effect_ScaleVec(scale);
            }
            model = &set->matSets[97];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Translation(uvMtx, 0.125f * (piece->cell & 1), 0.125f * (piece->cell >> 1), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            colorRow = Effect_OwnerColorRow(obj->ownerIndex);
            red = gEffectColors[colorRow].r;
            green = gEffectColors[colorRow].g;
            blue = gEffectColors[colorRow].b;
            if (piece->delay >= 8) {
                frac = (piece->delay - 8) / 8.0f;
                red = red - (u8) (red / 2 * frac);
                green = green - (u8) (green / 2 * frac);
                blue = blue - (u8) (blue / 2 * frac);
            }
            break;
        case 2:
            Gs_SetState(0x6C, 0);
            scale[0] = piece->scale * piece->sc[0];
            scale[1] = piece->scale * piece->sc[1];
            scale[2] = piece->scale * piece->sc[2];
            if (node->part == 0) {
                if ((obj->var0 & 0xF0) == 0x20) {
                    return;
                }
                rot[2] = (6.2831855f * obj->angle) / 65536.0f;
                model = &set->matSets[0];
            } else {
                rot[2] = (6.2831855f * piece->angle) / 65536.0f;
                model = &set->matSets[piece->idx] + 32;
            }
            if (obj->var1 == 1) {
                red = 0xFF;
                mode |= 2;
                green = 0xFF;
                blue = 0xFF;
                if (node->part == 0) {
                    Mtx_Translation(uvMtx, 0.125f, 0.0f, 0.0f);
                } else {
                    Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
                }
            } else {
                Effect_ScaleVec(scale);
                colorRow = Effect_OwnerColorRow(obj->ownerIndex);
                red = gEffectColors[colorRow].r;
                green = gEffectColors[colorRow].g;
                blue = gEffectColors[colorRow].b;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 5:
            Gs_SetState(0x6C, 0);
            scale[0] = piece->scale * piece->sc[0];
            scale[1] = piece->scale * piece->sc[1];
            scale[2] = piece->scale * piece->sc[2];
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            if (piece->idx == 0) {
                model = &set->matSets[0];
            } else {
                model = &set->matSets[piece->idx] + 31;
            }
            if (obj->var1 == 1) {
                red = 0xFF;
                mode |= 2;
                green = 0xFF;
                blue = 0xFF;
                if (piece->idx == 0) {
                    Mtx_Translation(uvMtx, 0.125f, 0.0f, 0.0f);
                } else {
                    Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
                }
            } else {
                Effect_ScaleVec(scale);
                colorRow = Effect_OwnerColorRow(obj->ownerIndex);
                red = gEffectColors[colorRow].r;
                green = gEffectColors[colorRow].g;
                blue = gEffectColors[colorRow].b;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 6:
            mode |= 2;
            scale[0] = piece->scale * piece->sc[0];
            scale[1] = piece->scale * piece->sc[1];
            scale[2] = piece->scale * piece->sc[2];
            if (node->part == 0) {
                rot[2] = (6.2831855f * piece->angle) / 65536.0f;
                model = &set->matSets[piece->idx] + 32;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
                Mtx_Translation(uvMtx, 0.25f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uvMtx);
            } else {
                model = &set->matSets[21];
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
            }
            break;
        case 8:
        case 15:
            mode |= 2;
            scale[0] = 0.5f * piece->scale * piece->sc[0];
            scale[1] = 0.5f * piece->scale * piece->sc[1];
            scale[2] = 0.5f * piece->scale * piece->sc[2];
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            model = &set->matSets[78];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_Translation(uvMtx, 0.25f * (piece->cell & 3), 0.25f * (piece->cell >> 2), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            break;
        case 9:
        case 12:
        case 13:
            Gs_SetState(0x6C, 0);
            if (obj->type == 9) {
                scale[0] = 0.5f * piece->scale * piece->sc[0];
                scale[1] = 0.5f * piece->scale * piece->sc[1];
                scale[2] = 0.5f * piece->scale * piece->sc[2];
            } else if (obj->type == 10 || obj->type == 13) {
                scale[0] = 0.2f * piece->scale * piece->sc[0];
                scale[1] = 0.2f * piece->scale * piece->sc[1];
                scale[2] = 0.2f * piece->scale * piece->sc[2];
            } else if (obj->type == 12) {
                scale[0] = 2.0f * piece->scale * piece->sc[0];
                scale[1] = 2.0f * piece->scale * piece->sc[1];
                scale[2] = 2.0f * piece->scale * piece->sc[2];
            }
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            if (obj->type == 12) {
                model = &set->matSets[74];
            } else {
                if (!(piece->idx & 1)) {
                    model = &set->matSets[77];
                    Mtx_Translation(uvMtx, 0.0f, 0.25f, 0.0f);
                } else {
                    model = &set->matSets[79];
                    Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                }
                Gs_SetState(0x19, (s32) uvMtx);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            break;
        case 14:
            scale[0] = piece->scale;
            scale[1] = piece->scale;
            scale[2] = piece->scale;
            rot[2] = (6.2831855f * piece->angle) / 65536.0f;
            model = &set->matSets[97];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
            Mtx_RotateX(mtx, 4.712389f);
            Mtx_Translation(uvMtx, 0.125f * (piece->cell & 1), 0.125f * (piece->cell >> 1), 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            red = *(u8 *)gEffectColors;
            green = gEffectColor0Green;
            blue = gEffectColor0Blue;
            break;
        }
        if (obj->type != 14) {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        Effect_DrawModel(model, mtx, blue | ((green << 8) | ((piece->alpha << 24) | (red << 16))), mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Returns a part's turn a frame by bits 1 and 2 of x (its idx): 0x111,
 * -0x110, 0x89 or -0x88.
 */
s32 HitSplash_SpinSpeed(s16 x)
{
    switch (x & 6) {
    case 0:
        return 0x111;
    case 2:
        return -0x110;
    case 4:
        return 0x89;
    case 6:
        return -0x88;
    }
    return 0;
}

/*
 * Plays the type's sound at pos from the common bank: the spray one of two by
 * var0 (none for 0x20; the first when its target's scale is at most 0.5); the
 * flashes, when var1 is 1, one of two by var0; the guard spark its own; type
 * 8 its own.
 */
void HitSplash_PlaySound(StageObj *obj, f32 *pos)
{
    switch (obj->type) {
    case 0:
        if (HitSplash_TargetScales[obj->var0 & 0xF] <= 0.5f) {
            Sound_PlaySeAtPosChecked(1, 0x6E, 0, pos, 1, 0);
        } else {
            switch (obj->var0 & 0xF0) {
            case 0:
                Sound_PlaySeAtPosChecked(1, 0x6E, 0, pos, 1, 0);
                break;
            case 0x10:
                Sound_PlaySeAtPosChecked(1, 0x6F, 0, pos, 1, 0);
                break;
            }
        }
        break;
    case 2:
    case 5:
        switch (obj->var1) {
        case 1:
            switch (obj->var0 & 0xF0) {
            case 0x10:
                Sound_PlaySeAtPosChecked(1, 0x65, 0, pos, 1, 0);
                break;
            case 0:
                Sound_PlaySeAtPosChecked(1, 0x64, 0, pos, 1, 0);
                break;
            }
            break;
        }
        break;
    case 6:
        Sound_PlaySeAtPosChecked(1, 0x63, 0, pos, 1, 0);
        break;
    case 8:
        Sound_PlaySeAtPosChecked(1, 0x95, 0, pos, 1, 0);
        break;
    }
}

/*
 * Creates a HitSplash of type k where hit lands on em (a monster or a
 * hunter), when em is in the current area, with size scale; hitKind (the
 * hit's ObjAttach.hitKind) sets var0's high bits (0 gives 0x20, 3 gives 0x10,
 * others 0). The guard spark (6) stays at pos with a random angle. The others
 * face back toward the hitter (the hit's angle + 0x8000). On a monster,
 * ownerIndex keeps its kind (the colour), five kinds (Melynx, Vespoid and
 * Hornetaur among them) take scale row 2, and the object follows the bone of
 * the hit part, pos kept relative to it, or stays at pos when no part was
 * hit; on a hunter it takes row 1 and follows bone 0xA (types 8 and 9 at the
 * bone itself).
 */
void HitSplash_CreateOnEm(Em *em, ObjAttach *hit, u8 k, s16 hitKind, f32 *pos, f32 scale)
{
    StageObj *obj;
    f32 boneMtx[16];
    f32 invMtx[16];
    f32 bonePos[3];

    if (em != NULL && (Obj_InCurrentArea(em) & 0xFF)) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_HIT_SPLASH;
            obj->update = HitSplash_Update;
            obj->type = k;
            obj->owner = em;
            obj->ownerIndex = 0;
            switch (hitKind) {
            case 2:
            default:
                obj->var0 = 0;
                break;
            case 3:
                obj->var0 = 0x10;
                break;
            case 0:
                obj->var0 = 0x20;
                break;
            }
            obj->scale = scale;
            if (obj->type == 6) {
                obj->pos[0] = pos[0];
                obj->pos[1] = pos[1];
                obj->pos[2] = pos[2];
                obj->angle = Rand_Next(1);
                obj->var2 = 0xFF;
                return;
            }
            obj->angle = em->hitAngle + 0x7FFF + 1;
            obj->var0 &= 0xF0;
            if (em->isEm != 0) {
                obj->ownerIndex = em->type;
                switch (obj->ownerIndex) {
                case 9:
                case 0x17:
                case 0x13:
                case 0x18:
                case 0x38:
                    obj->var0 |= 2;
                    break;
                }
                if (hit != NULL && hit->targetPart != NULL) {
                    obj->var2 = hit->targetPart->bone;
                    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, obj->var2)));
                    Mtx_GetTrans(bonePos, boneMtx);
                    obj->pos[0] = pos[0] - bonePos[0];
                    obj->pos[1] = pos[1] - bonePos[1];
                    obj->pos[2] = pos[2] - bonePos[2];
                    Mtx_Inverse(invMtx, boneMtx);
                    Vec_RotateSelf(obj->pos, invMtx);
                    return;
                }
            } else {
                obj->var0 |= 1;
                obj->var2 = 0xA;
                if (obj->type == 8 || obj->type == 9) {
                    obj->pos[0] = 0.0f;
                    obj->pos[1] = 0.0f;
                    obj->pos[2] = 0.0f;
                } else {
                    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, obj->var2)));
                    Mtx_GetTrans(bonePos, boneMtx);
                    obj->pos[0] = pos[0] - bonePos[0];
                    obj->pos[1] = pos[1] - bonePos[1];
                    obj->pos[2] = pos[2] - bonePos[2];
                    Mtx_Inverse(invMtx, boneMtx);
                    Vec_RotateSelf(obj->pos, invMtx);
                }
                return;
            }
            obj->var2 = 0xFF;
            obj->pos[0] = pos[0];
            obj->pos[1] = pos[1];
            obj->pos[2] = pos[2];
        }
    }
}

/*
 * Creates a HitSplash of type k at pos with the given life (angle), part
 * spacing (var1) and size scale, following no bone, in any area.
 * ItemAttack_SetupPlaced makes type 13 (75 frames, 10 apart).
 */
void HitSplash_CreateAt(f32 *pos, u8 k, s16 life, u8 spacing, f32 scale)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_HIT_SPLASH;
        obj->update = HitSplash_Update;
        obj->type = k;
        obj->angle = life;
        obj->scale = scale;
        obj->var1 = spacing;
        obj->var2 = 0xFF;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
    }
}

/*
 * Creates a HitSplash of type k at pos for em (when in the current area) with
 * size scale, a random angle and no bone; hitKind sets var0 as in
 * HitSplash_CreateOnEm. hitFxKind (the hit's hitFx & HITFX_KIND_MASK) 6 to 17
 * sets var1 1 (drawn white, with sound); otherwise the object takes em's kind
 * as its colour (ownerIndex).
 */
void HitSplash_CreateByHitFx(Em *em, f32 *pos, u8 k, s16 hitKind, s16 hitFxKind, f32 scale)
{
    StageObj *obj;

    if (em != NULL && (Obj_InCurrentArea(em) & 0xFF)) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = STAGEOBJ_KIND_HIT_SPLASH;
            obj->update = HitSplash_Update;
            obj->type = k;
            obj->owner = em;
            obj->ownerIndex = 0;
            switch (hitKind) {
            case 2:
            default:
                obj->var0 = 0;
                break;
            case 3:
                obj->var0 = 0x10;
                break;
            case 0:
                obj->var0 = 0x20;
                break;
            }
            obj->var0 &= 0x3F;
            switch (hitFxKind) {
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                obj->var1 = 1;
                break;
            default:
                obj->var1 = 0;
                if (em->isEm != 0) {
                    obj->ownerIndex = em->type;
                }
                break;
            }
            obj->scale = scale;
            obj->pos[0] = pos[0];
            obj->pos[1] = pos[1];
            obj->pos[2] = pos[2];
            obj->angle = Rand_Next(1);
            obj->var2 = 0xFF;
        }
    }
}

/*
 * Creates a HitSplash of type k at pos with size scale, a random angle, no
 * owner and no bone, in any area.
 */
void HitSplash_CreateAtScaled(f32 *pos, u8 k, f32 scale)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = STAGEOBJ_KIND_HIT_SPLASH;
        obj->update = HitSplash_Update;
        obj->type = k;
        obj->owner = NULL;
        obj->ownerIndex = 0;
        obj->scale = scale;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        obj->angle = Rand_Next(1);
        obj->var2 = 0xFF;
    }
}

/* Returns a random number from 0 to 1.023 (ten random bits times 0.001). */
f32 StageObj_RandUnit(void)
{
    return 0.001f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
}

/* Returns a random number from -0.512 to 0.511. */
f32 StageObj_RandSigned(void)
{
    return 0.001f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

/*
 * AttackBurst's update, run every frame: switches on obj->state to its set-
 * up, run, end and free states.
 */
void AttackBurst_Update(StageObj *obj)
{
    switch (obj->state) {
    case STAGEOBJ_SETUP:
        AttackBurst_Setup(obj);
        break;
    case STAGEOBJ_RUN:
        AttackBurst_Run(obj);
        break;
    case STAGEOBJ_END:
        AttackBurst_End(obj);
        break;
    case STAGEOBJ_FREE:
        AttackBurst_Free(obj);
        break;
    }
}

/*
 * The set-up state: visible; the rocks (type 8) run AttackBurst_SetupRocks,
 * the other types AttackBurst_SetupParts.
 */
void AttackBurst_Setup(StageObj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    case 8:
        AttackBurst_SetupRocks(obj);
        break;
    default:
        AttackBurst_SetupParts(obj);
        break;
    }
}

/*
 * The run state: the rocks run AttackBurst_RunRocks, the others
 * AttackBurst_RunParts.
 */
void AttackBurst_Run(StageObj *obj)
{
    switch (obj->type) {
    case 8:
        AttackBurst_RunRocks(obj);
        break;
    default:
        AttackBurst_RunParts(obj);
        break;
    }
}

/*
 * The end state: the rocks run AttackBurst_EndRocks, the others
 * AttackBurst_EndParts.
 */
void AttackBurst_End(StageObj *obj)
{
    switch (obj->type) {
    case 8:
        AttackBurst_EndRocks(obj);
        break;
    default:
        AttackBurst_EndParts(obj);
        break;
    }
}

/*
 * The draw callback of every part: the rocks' AttackBurst_DrawRock, the
 * others' AttackBurst_DrawPart.
 */
void AttackBurst_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    case 8:
        AttackBurst_DrawRock(node);
        break;
    default:
        AttackBurst_DrawPart(node);
        break;
    }
}

/* a part of an AttackBurst of the types other than the rocks (0x30 bytes, in its blocks) */
typedef struct AttackBurstPart {
    s16 slot;                   /* 0x00: its draw slot */
    s16 idx;                    /* 0x02 */
    f32 sc[3];                  /* 0x04: its scale, from a curve by its frame */
    f32 pos[3];                 /* 0x10 */
    f32 alpha;                  /* 0x1C */
    s16 delay;                  /* 0x20: its frame, counting up from a negative delay */
    s16 spin;                   /* 0x22 */
    u16 yaw;                  /* 0x24 */
    u16 angle;                  /* 0x26 */
    f32 scale;                  /* 0x28 */
    DrawSlot *node;                /* 0x2C */
} AttackBurstPart;
extern s16 AttackBurst_DelaysA[];
extern s16 AttackBurst_DelaysB[];
void AttackBurst_PlaceAtBone(StageObj *o);
s32 AttackBurst_OwnerInMotion(StageObj *o);
f32 StageObj_RandUnit(void);
f32 StageObj_RandSigned(void);
extern s16 AttackBurst_PartCounts[];
void Mtx_TurnX(f32 *m, f32 a);
extern u16 AttackBurst_Durations[];
extern s16 AttackBurst_CurveIdx[];
extern s16 AttackBurst_Steps[];
extern u16 *AttackBurst_Tbls[];
extern void *AttackBurst_Curves[];
void Vec_RotateX(f32 *v, f32 a);

/*
 * Sets up the types other than the rocks. First the object: type 0 moves 50
 * ahead along its yaw; the fireball flares (1, 2) move in front of a bone of
 * the owner (0x33 for the Fatalis kinds, 0x13 for Remobra, else 0x22), keep
 * its kind and face its yaw; a trail (3) takes its pitch from the timer,
 * keeps its yaw for the parts and runs var0 parts (at least 1); type 7 goes
 * to the owner's bone while it is in the motion (AttackBurst_OwnerInMotion,
 * AttackBurst_PlaceAtBone), else var1 0xFF; the spray (12) starts 50 (times
 * the monster's depth scale) along bone `angle` and moves 10 a frame along
 * that bone's axis turned down by var0 << 8; types 12 and 14 keep the owner's
 * kind (14 at double size); type 18 a random spin in var1. Then each part
 * (AttackBurst_PartCounts[type]) gets its delay, angle, size and place by
 * type and a draw slot that AttackBurst_Draw draws, and the first frame
 * runs.
 */
void AttackBurst_SetupParts(StageObj *obj)
{
    AttackBurstPart *piece = (AttackBurstPart *) obj->buf;
    s16 count;
    s16 i;
    u16 trailYaw;
    f32 boneMtx[16];
    f32 offset[3];

    count = AttackBurst_PartCounts[obj->type];
    obj->var1 = 0;
    switch (obj->type) {
    case 0:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 50.0f;
        Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        break;
    case 1:
        obj->var2 = ((Em *) obj->owner)->type;
        offset[0] = 0.0f;
        if (obj->var2 == 2 || obj->var2 == 0x24 || obj->var2 == 0x47) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x33)));
            offset[1] = -41.8f;
            offset[2] = 50.0f;
        } else if (obj->var2 == 0x3F) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x13)));
            offset[1] = -50.0f;
            offset[2] = 50.0f;
        } else {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x22)));
            offset[1] = -80.0f;
            offset[2] = 80.0f;
        }
        Vec_RotateSelf(offset, boneMtx);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        obj->angle = ((Em *) obj->owner)->rotY;
        break;
    case 2:
        obj->var2 = ((Em *) obj->owner)->type;
        offset[0] = 0.0f;
        if (obj->var2 == 2 || obj->var2 == 0x24 || obj->var2 == 0x47) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x33)));
            offset[1] = -91.6f;
            offset[2] = 132.8f;
        } else if (obj->var2 == 0x3F) {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x13)));
            offset[1] = -50.0f;
            offset[2] = 50.0f;
        } else {
            Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, 0x22)));
            offset[1] = -80.0f;
            offset[2] = 120.0f;
        }
        Vec_RotateSelf(offset, boneMtx);
        obj->pos[0] += offset[0];
        obj->pos[1] += offset[1];
        obj->pos[2] += offset[2];
        obj->angle = ((Em *) obj->owner)->rotY;
        break;
    case 3:
        if (obj->var0 == 0) {
            count = 1;
        } else {
            count = obj->var0;
        }
        trailYaw = obj->angle;
        obj->angle = obj->timer;
        obj->timer = 0;
        break;
    case 6:
        obj->angle = Rand_Next(1);
        break;
    case 7:
        if (AttackBurst_OwnerInMotion(obj) == 1) {
            AttackBurst_PlaceAtBone(obj);
        } else {
            obj->var1 = 0xFF;
        }
        obj->angle = Rand_Next(1);
        break;
    case 12:
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(obj->owner, obj->angle)));
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 50.0f * ((Em *) obj->owner)->scaleZ;
        Vec_RotateSelf(offset, boneMtx);
        offset[0] += obj->pos[0];
        offset[1] += obj->pos[1];
        offset[2] += obj->pos[2];
        Mtx_TurnX(boneMtx, 6.2831855f * (obj->var0 << 8) / 65536.0f);
        Vec_Copy(obj->pos, &boneMtx[8]);
        obj->pos[0] *= 10.0f;
        obj->pos[1] *= 10.0f;
        obj->pos[2] *= 10.0f;
        obj->var2 = ((Em *) obj->owner)->type;
        break;
    case 14:
        obj->var2 = ((Em *) obj->owner)->type;
        obj->scale *= 2.0f;
        break;
    case 5:
    case 16:
    case 17:
        break;
    case 18:
        obj->var1 = Rand_Next(1) & 0x1FF;
        break;
    }
    obj->timer = 0;
    for (i = 0; i < count; piece++, i++) {
        piece->idx = i;
        switch (obj->type) {
        case 0:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            piece->scale = 1.0f;
            break;
        case 1:
            switch (i) {
            case 0:
            case 1:
                piece->idx = 0;
                piece->delay = 0;
                break;
            case 2:
            case 3:
                piece->idx = 1;
                piece->delay = -2;
                break;
            }
            piece->scale = obj->scale * (1.0f + 0.5f * StageObj_RandUnit());
            piece->yaw = Rand_Next(1);
            piece->angle = Rand_Next(1);
            piece->spin = (Rand_Next(1) & 0xFF) - 0x80;
            break;
        case 2:
            if (i == 0) {
                piece->delay = 0;
            } else {
                piece->delay = -1;
            }
            piece->angle = Rand_Next(1);
            piece->scale = 0.0f;
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 3:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            piece->yaw = trailYaw;
            if (obj->var2 == 2 || obj->var2 == 0x24 || obj->var2 == 0x47) {
                piece->scale = 1.5f;
            } else {
                piece->scale = 1.0f;
            }
            if (i == 0) {
                piece->scale *= 1.0f + StageObj_RandUnit();
                piece->spin = (Rand_Next(1) & 0xFF) - 0x80;
            } else {
                piece->scale *= 1.0f + 1.5f * StageObj_RandUnit();
                piece->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            }
            break;
        case 4:
            piece->delay = -5 - i * 2;
            piece->angle = Rand_Next(1);
            piece->spin = 0.5f + 65536.0f * StageObj_RandSigned() / 360.0f;
            piece->scale = obj->scale * (1.0f + 0.5f * StageObj_RandUnit());
            piece->pos[0] = obj->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            piece->pos[1] = obj->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            piece->pos[2] = i * -10;
            break;
        case 5:
        case 16:
            piece->delay = -4 - i * 5;
            piece->idx = i;
            piece->angle = Rand_Next(1);
            piece->scale = obj->scale * (1.0f + 0.5f * StageObj_RandUnit());
            piece->spin = 0.5f + 65536.0f * (2.0f * StageObj_RandSigned()) / 360.0f;
            piece->pos[0] = 3.3333333f * (2.0f * StageObj_RandSigned());
            piece->pos[1] = 7.5f;
            piece->pos[2] = 0.0f;
            break;
        case 6:
            piece->delay = 0;
            piece->angle = 0;
            piece->scale = 1.0f;
            piece->alpha = 1.0f;
            break;
        case 7:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            piece->scale = 1.0f;
            piece->alpha = 1.0f;
            break;
        case 9:
            piece->delay = -i;
            piece->angle = Rand_Next(1);
            break;
        case 10:
            piece->delay = -2;
            piece->pos[0] = obj->pos[0];
            piece->pos[1] = 2.0f + obj->pos[1];
            piece->pos[2] = obj->pos[2];
            break;
        case 11:
            piece->delay = AttackBurst_DelaysA[i];
            piece->scale = 1.2f * obj->scale;
            break;
        case 12:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            piece->scale = 0.0f;
            Vec_Copy(piece->pos, offset);
            break;
        case 13:
            piece->delay = 0;
            piece->idx = Rand_Next(1) & 3;
            piece->angle = 0;
            piece->scale = obj->scale * (0.8f + 0.4f * StageObj_RandUnit());
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 14:
            piece->delay = 0;
            piece->idx = Rand_Next(1) & 1;
            piece->angle = Rand_Next(1);
            piece->scale = obj->scale * (0.8f + 0.4f * StageObj_RandUnit());
            piece->pos[0] = obj->pos[0] + 5.0f * StageObj_RandSigned();
            piece->pos[1] = obj->pos[1] + 5.0f * StageObj_RandSigned();
            piece->pos[2] = obj->pos[2] + 5.0f * StageObj_RandSigned();
            break;
        case 15:
            piece->delay = AttackBurst_DelaysB[i];
            if (i == 0) {
                piece->idx = 0;
            } else {
                piece->idx = Rand_Next(1) % 6 + 1;
            }
            piece->angle = 0;
            Vec_Copy(piece->pos, obj->pos);
            piece->scale = obj->scale * (0.8f + 0.4f * StageObj_RandUnit());
            break;
        case 17:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 18:
            piece->delay = 0;
            piece->angle = Rand_Next(1);
            piece->scale = 0.8f + 0.2f * StageObj_RandUnit();
            break;
        case 19:
            piece->delay = i * -3;
            piece->angle = Rand_Next(1);
            piece->scale = 0.0f;
            Vec_Copy(piece->pos, offset);
            piece->pos[0] += (Rand_Next(1) & 0x1F) - 0x10;
            piece->pos[1] += (Rand_Next(1) & 0x1F) - 0x10;
            piece->pos[2] += (Rand_Next(1) & 0x1F) - 0x10;
            break;
        case 20:
            piece->delay = i * -3;
            piece->idx = Rand_Next(1) & 3;
            piece->angle = 0;
            piece->scale = obj->scale * (0.8f + 0.4f * StageObj_RandUnit());
            Vec_Copy(piece->pos, obj->pos);
            piece->pos[0] += (Rand_Next(1) & 0x1F) - 0x10;
            piece->pos[1] += (Rand_Next(1) & 0x1F) - 0x10;
            piece->pos[2] += (Rand_Next(1) & 0x1F) - 0x10;
            break;
        }
        piece->slot = DrawSlot_Alloc();
        if (piece->slot != -1) {
            piece->node = DrawSlot_Get(piece->slot);
            piece->node->owner = obj;
            piece->node->part = i;
            piece->node->draw = AttackBurst_Draw;
        } else {
            piece->node = NULL;
        }
    }
    AttackBurst_Run(obj);
}

/*
 * Runs a frame of the types other than the rocks; the object ends at
 * AttackBurst_Durations[type]. The trail runs var0 parts; type 7 follows the
 * owner's bone while it stays in the motion; type 14 follows the owner's bone
 * `angle` and ends with the owner. Each part waits out its delay and lives
 * `life` frames (by type, or per part from AttackBurst_PartLives for types
 * 2, 11 and 15; 10 for the trail's head, 35 its puffs; 15 and 20 for type 9),
 * takes its scale (types 1, 11, 14 and 15 also a path) from
 * AttackBurst_Curves by its frame, with two curves a part its opacity too,
 * and is placed: type 0 runs 80 along the yaw, the flares follow their path
 * (mirrored for parts 1 and 3), the trail's head and puffs run along its
 * pitch and yaw, the spray moves on, the others stack toward the camera or
 * stay. Type 15 adds the path's x to all three coordinates, as the game's
 * code does. Each part goes into the sorted draw table.
 */
void AttackBurst_RunParts(StageObj *obj)
{
    AttackBurstPart *piece;
    s16 i;
    u16 dur;
    s16 k;
    s16 step;
    u16 *lives;
    u16 life;
    s16 count;
    Em *em;
    void *curve;
    f32 boneMtx[16];
    f32 offset[3];

    piece = (AttackBurstPart *) obj->buf;
    count = AttackBurst_PartCounts[obj->type];
    dur = AttackBurst_Durations[obj->type];
    k = AttackBurst_CurveIdx[obj->type];
    step = AttackBurst_Steps[obj->type];
    lives = AttackBurst_Tbls[obj->type];

    switch (obj->type) {
    case 1:
        life = 0x12;
        break;
    case 3:
        if (obj->var0 == 0) {
            count = 1;
        } else {
            count = obj->var0;
        }
        break;
    case 4:
        life = 0x26;
        break;
    case 5:
    case 16:
        life = 0x18;
        break;
    case 7:
        if (obj->var1 != 0xFF) {
            if (AttackBurst_OwnerInMotion(obj) == 1) {
                AttackBurst_PlaceAtBone(obj);
            } else {
                obj->var1 = 0xFF;
            }
        }
        life = dur;
        break;
    case 10:
        life = 6;
        break;
    case 0:
    case 6:
    case 12:
    case 13:
    case 17:
    case 18:
        life = dur;
        break;
    case 14:
        em = obj->owner;
        life = dur;
        if (em->taskPhase == 3 || em->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(em, obj->angle)));
        offset[0] = 0.0f;
        offset[1] = -50.0f;
        offset[2] = 100.0f;
        Vec_RotateSelf(offset, boneMtx);
        obj->pos[0] = boneMtx[12] + offset[0];
        obj->pos[1] = boneMtx[13] + offset[1];
        obj->pos[2] = boneMtx[14] + offset[2];
        break;
    case 19:
        life = 0xF;
        break;
    case 20:
        life = 0x12;
        break;
    }
    if (++obj->timer >= dur) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    for (i = 0; i < count; i++) {
        switch (obj->type) {
        case 1:
        case 4:
        case 5:
        case 16:
        case 19:
        case 20:
            k = AttackBurst_CurveIdx[obj->type];
            break;
        case 2:
        case 11:
        case 15:
            life = lives[i];
            break;
        case 3:
            if (i == 0) {
                life = 10;
            } else {
                life = 35;
            }
            break;
        case 9:
            if (i == 0) {
                life = 15;
            } else {
                life = 20;
            }
            break;
        }
        if (++piece->delay <= 0) {
            piece++;
            k += step;
            continue;
        }
        if (piece->delay >= life) {
            piece++;
            k += step;
            continue;
        }
        curve = AttackBurst_Curves[k++];
        Key_LerpVec3(curve, piece->sc, piece->delay);
        switch (obj->type) {
        case 0:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 80.0f * piece->delay / life;
            Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
            piece->pos[0] = obj->pos[0] + offset[0];
            piece->pos[1] = obj->pos[1] + offset[1];
            piece->pos[2] = obj->pos[2] + offset[2];
            break;
        case 5:
        case 16:
            piece->angle = piece->angle + (u16) piece->spin;
            break;
        case 1:
            curve = AttackBurst_Curves[k++];
            Key_LerpVec3(curve, piece->pos, piece->delay);
            switch (piece->idx) {
            case 0:
                break;
            case 1:
                piece->pos[0] -= 40.0f;
                break;
            }
            if (i == 1 || i == 3) {
                piece->pos[0] = -piece->pos[0];
            }
            piece->pos[0] *= obj->scale;
            piece->pos[1] *= obj->scale;
            piece->pos[2] *= obj->scale;
            Vec_RotateY(piece->pos, 6.2831855f * obj->angle / 65536.0f);
            piece->pos[0] += obj->pos[0];
            piece->pos[1] += obj->pos[1];
            piece->pos[2] += obj->pos[2];
            piece->angle = piece->angle + (u16) piece->spin;
            break;
        case 2:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 5.0f * i;
            Vec_RotateSelf(offset, gCamMatrix);
            piece->pos[0] = obj->pos[0] + offset[0];
            piece->pos[1] = obj->pos[1] + offset[1];
            piece->pos[2] = obj->pos[2] + offset[2];
            break;
        case 3:
            piece->pos[0] = 0.0f;
            piece->pos[1] = 0.0f;
            if (i == 0) {
                piece->pos[2] = 120.0f - 50.0f * piece->delay / life;
            } else {
                piece->pos[2] = -(30.0f * piece->delay / life);
            }
            piece->angle = piece->angle + (u16) piece->spin;
            Vec_RotateX(piece->pos, 6.2831855f * obj->angle / 65536.0f);
            Vec_RotateY(piece->pos, 6.2831855f * piece->yaw / 65536.0f);
            piece->pos[0] += obj->pos[0];
            piece->pos[1] += obj->pos[1];
            piece->pos[2] += obj->pos[2];
            break;
        case 4:
            Vec_Rotate(offset, piece->pos, gCamMatrix);
            piece->angle = piece->angle + (u16) piece->spin;
            break;
        case 6:
            piece->scale = obj->scale * (1.0f + 0.2f * StageObj_RandUnit());
            Vec_Copy(piece->pos, obj->pos);
            break;
        case 7:
            piece->angle = Rand_Next(1);
            piece->scale = obj->scale * (1.0f + 0.2f * StageObj_RandUnit());
            Vec_Copy(piece->pos, obj->pos);
            if (piece->delay > 23) {
                piece->alpha = (f32) (life - piece->delay) / 13.0f;
            }
            break;
        case 9:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 20.0f + i * 3;
            Vec_RotateSelf(offset, gCamMatrix);
            piece->pos[0] = obj->pos[0] + offset[0];
            piece->pos[1] = obj->pos[1] + offset[1];
            piece->pos[2] = obj->pos[2] + offset[2];
            break;
        case 10:
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = -1000.0f;
            Vec_RotateSelf(offset, gCamMatrix);
            piece->pos[0] = obj->pos[0] + offset[0];
            piece->pos[1] = obj->pos[1] + offset[1];
            piece->pos[2] = obj->pos[2] + offset[2];
            break;
        case 11:
            curve = AttackBurst_Curves[k++];
            Key_LerpVec3(curve, piece->pos, piece->delay);
            piece->pos[0] *= obj->scale;
            piece->pos[1] *= obj->scale;
            piece->pos[2] *= obj->scale;
            piece->pos[0] += obj->pos[0];
            piece->pos[1] += obj->pos[1];
            piece->pos[2] += obj->pos[2];
            break;
        case 12:
            piece->pos[0] += obj->pos[0];
            piece->pos[1] += obj->pos[1];
            piece->pos[2] += obj->pos[2];
            break;
        case 14:
            curve = AttackBurst_Curves[k++];
            Key_LerpVec3(curve, piece->pos, piece->delay);
            Vec_RotateSelf(piece->pos, boneMtx);
            piece->pos[0] += obj->pos[0];
            piece->pos[1] += obj->pos[1];
            piece->pos[2] += obj->pos[2];
            break;
        case 15:
            curve = AttackBurst_Curves[k++];
            Key_LerpVec3(curve, offset, piece->delay);
            Vec_RotateY(offset, 6.2831855f * obj->angle / 65536.0f);
            piece->pos[0] = 0.0f;
            piece->pos[1] = 0.0f;
            piece->pos[2] = 5.0f * i;
            Vec_RotateSelf(piece->pos, gCamMatrix);
            piece->pos[0] += obj->pos[0] + offset[0];
            piece->pos[1] += obj->pos[1] + offset[0];
            piece->pos[2] += obj->pos[2] + offset[0];
            break;
        case 18:
            piece->angle += obj->var1 - 0x100;
            piece->pos[0] = obj->pos[0];
            piece->pos[1] = obj->pos[1];
            piece->pos[2] = obj->pos[2];
            break;
        case 19:
        case 20:
            piece->pos[0] = obj->pos[0];
            piece->pos[1] = obj->pos[1];
            piece->pos[2] = obj->pos[2];
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 5.0f * i;
            Vec_RotateSelf(offset, gCamMatrix);
            break;
        }
        if (step > 1 && obj->type != 15) {
            curve = AttackBurst_Curves[k++];
            Key_LerpFloat(curve, &piece->alpha, piece->delay);
        }
        if (piece->node != NULL) {
            if (obj->type == 5 || obj->type == 16) {
                piece->node->pos[0] = obj->pos[0] + piece->delay * piece->pos[0];
                piece->node->pos[1] = obj->pos[1] + piece->delay * piece->pos[1];
                piece->node->pos[2] = obj->pos[2] + piece->delay * piece->pos[2];
            } else if (obj->type == 4 || obj->type == 19 || obj->type == 20) {
                piece->node->pos[0] = obj->pos[0] + offset[0];
                piece->node->pos[1] = obj->pos[1] + offset[1];
                piece->node->pos[2] = obj->pos[2] + offset[2];
            } else {
                piece->node->pos[0] = piece->pos[0];
                piece->node->pos[1] = piece->pos[1];
                piece->node->pos[2] = piece->pos[2];
            }
            OT_AddByDepth(gDrawOtSorted, piece->node, 0x40, 0);
        }
        piece++;
    }
}

/*
 * Draws one part of the types other than the rocks, scaled and turned about
 * the view axis. By type the model comes from the stage set or from the owner
 * kind's own part models (gScene.partModels of EmSlot_FindKind(var2); nothing
 * is drawn when they are not loaded): the flares orange and white, the
 * trail's head and puffs by colour keys (the Fatalis kinds their own), the
 * impacts dark or white, the fireball impact puffs (5, 16) by colour keys,
 * the sprays white for the Plesioth kinds and sand-coloured for the others,
 * the Gypceros kinds their own colour keys for types 15 and 18. The ground
 * mark (10) lies flat 2 above the object, not turned to the camera. The test
 * for type 0 in the case of types 5 and 16 never holds.
 */
void AttackBurst_DrawPart(DrawSlot *node)
{
    StageObj *obj;
    AttackBurstPart *piece;
    ModelMatSet *model;
    u16 mode;
    f32 mtx[16];
    ModelMat *mats;
    void *curve;
    s16 emSlot;
    u32 color;
    f32 rot[3];
    f32 scale[3];
    f32 groundPos[3];
    f32 uvMtx[16];
    Model *set;

    mode = 0;
    obj = node->owner;
    piece = (AttackBurstPart *) obj->buf + node->part;
    set = gStageModelSet;
    if (set == NULL || !set->active) {
        goto end;
    }
    mats = set->mats;
    switch (obj->type) {
    case 0:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        if (obj->var2 == 0x35) {
            model = &set->matSets[3];
        } else {
            model = &set->matSets[4];
        }
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = obj->scale * piece->sc[0];
        scale[1] = obj->scale * piece->sc[1];
        scale[2] = obj->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 1:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        mode |= 2;
        model = &set->matSets[2];
        Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xF4902E;
        break;
    case 2:
        if (piece->idx == 0) {
            model = &set->matSets[66];
        } else {
            model = &set->matSets[80];
        }
        mode |= 2;
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = obj->scale * piece->sc[0];
        scale[1] = obj->scale * piece->sc[1];
        scale[2] = obj->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 3:
        if (piece->idx == 0) {
            emSlot = EmSlot_FindKind(obj->var2);
            if (emSlot == -1) {
                goto end;
            }
            set = gScene.partModels[emSlot];
            if (set == NULL || !set->active) {
                goto end;
            }
            model = &set->matSets[2];
            mats = set->mats;
            mode |= 2;
            Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) uvMtx);
            if (obj->var2 == 2 || obj->var2 == 0x24 || obj->var2 == 0x47) {
                curve = AttackBurst_Curve3B;
            } else {
                curve = AttackBurst_Curve3A;
            }
            Color_EvalKeys(curve, piece->delay, &color);
        } else {
            model = &set->matSets[72];
            Color_EvalKeys((s32 *) AttackBurst_ColorKeys, piece->delay, &color);
        }
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->sc[0] * piece->scale;
        scale[1] = piece->sc[1] * piece->scale;
        scale[2] = piece->sc[2] * piece->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        break;
    case 4:
        scale[0] = piece->sc[0] * piece->scale;
        scale[1] = piece->sc[1] * piece->scale;
        scale[2] = piece->sc[2] * piece->scale;
        model = &set->matSets[74];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0x111111;
        break;
    case 5:
    case 16:
        model = &set->matSets[74];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        if (obj->type == 0) {
            color = ((u8) (255.0f * piece->alpha) << 24) | 0x333333;
        } else {
            if (obj->type == 5) {
                curve = AttackBurst_Curve10A;
            } else {
                curve = AttackBurst_Curve10B;
            }
            Color_EvalKeys(curve, piece->delay, &color);
        }
        break;
    case 6:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        if (obj->var2 == 0x35) {
            model = &set->matSets[5];
        } else {
            model = &set->matSets[8];
        }
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 7:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        model = &set->matSets[2];
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 9:
        mode |= 2;
        if (node->part == 0) {
            model = &set->matSets[59];
        } else {
            model = &set->matSets[81];
        }
        scale[0] = 5.0f * obj->scale * piece->sc[0];
        scale[1] = 5.0f * obj->scale * piece->sc[1];
        scale[2] = 5.0f * obj->scale * piece->sc[2];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 10:
        mode |= 2;
        scale[0] = obj->scale * piece->sc[0];
        scale[1] = obj->scale * piece->sc[1];
        scale[2] = obj->scale * piece->sc[2];
        model = &set->matSets[42];
        groundPos[0] = obj->pos[0];
        groundPos[1] = 2.0f + obj->pos[1];
        groundPos[2] = obj->pos[2];
        Gs_SetState(0x6C, 0);
        Mtx_FromScaleRotTrans(scale, rot, groundPos, 0, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 11:
        Gs_SetState(0x6C, 0);
        model = &set->matSets[35];
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0x111111;
        break;
    case 12:
    case 19:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        if (obj->var2 == 0x15 || obj->var2 == 0x2E) {
            mode |= 2;
            color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        } else {
            color = ((u8) (255.0f * piece->alpha) << 24) | 0xD8B68A;
        }
        model = &set->matSets[4];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = obj->scale * piece->sc[0];
        scale[1] = obj->scale * piece->sc[1];
        scale[2] = obj->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        break;
    case 13:
    case 20:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        if (obj->var2 == 0x15 || obj->var2 == 0x2E) {
            mode |= 2;
            color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        } else {
            color = ((u8) (255.0f * piece->alpha) << 24) | 0xD8B68A;
        }
        model = &set->matSets[piece->idx];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        break;
    case 14:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        model = &set->matSets[0];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xF8D197;
        break;
    case 15:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        if (piece->idx == 0) {
            curve = AttackBurst_CurveFA;
            model = &set->matSets[3];
        } else {
            model = set->matSets + piece->idx % 3 + 4;
            if (obj->var2 == 0x14 || obj->var2 == 0x27) {
                curve = AttackBurst_CurveFB;
            } else {
                curve = AttackBurst_CurveFC;
            }
            if (piece->idx & 1) {
                Mtx_TurnY(mtx, 3.1415927f);
            }
        }
        Color_EvalKeys(curve, piece->delay, &color);
        break;
    case 17:
        mode |= 2;
        model = &set->matSets[81];
        scale[0] = obj->scale * piece->sc[0];
        scale[1] = obj->scale * piece->sc[1];
        scale[2] = obj->scale * piece->sc[2];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        color = ((u8) (255.0f * piece->alpha) << 24) | 0xFFFFFF;
        break;
    case 18:
        emSlot = EmSlot_FindKind(obj->var2);
        if (emSlot == -1) {
            goto end;
        }
        set = gScene.partModels[emSlot];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        model = &set->matSets[0];
        scale[0] = piece->scale * piece->sc[0];
        scale[1] = piece->scale * piece->sc[1];
        scale[2] = piece->scale * piece->sc[2];
        rot[2] = (6.2831855f * piece->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, mtx);
        if (obj->var2 == 0x14 || obj->var2 == 0x27) {
            curve = AttackBurst_Curve12A;
        } else {
            curve = AttackBurst_Curve12B;
        }
        Color_EvalKeys(curve, piece->delay, &color);
        break;
    }
    if (obj->type != 10) {
        Mtx_Concat3x3(mtx, gCamMatrix);
    }
    Effect_DrawModel(model, mtx, color, mode, mats);
    Gs_SetState(0x6C, 1);
    Draw_SetDefaultState();
end:;
}

/* The file's data. */
u32 ItemProp_PartCurves_0[24] = {
#include "game/008CE640.inc"
};
u32 ItemProp_PartCurves_1[12] = {
#include "game/008CE6A0.inc"
};
u32 ItemProp_PartCurves_2[24] = {
#include "game/008CE6D0.inc"
};
u32 ItemProp_PartCurves_3[8] = {
#include "game/008CE730.inc"
};
void *ItemProp_PuffCurves[4] = {
    (void *)ItemProp_PartCurves_0, (void *)ItemProp_PartCurves_1, (void *)ItemProp_PartCurves_2, (void *)ItemProp_PartCurves_3,
};
ColorRgba ItemProp_SmokeColors[8] = {
#include "game/008CE760.inc"
};
u32 ItemProp_BaitColorsFirst[4] = {
#include "game/008CE780.inc"
};
u32 ItemProp_BaitColorsSecond[4] = {
#include "game/008CE790.inc"
};
u32 FireBlast_PartCurves_0[16] = {
#include "game/008CE7A0.inc"
};
u32 FireBlast_PartCurves_2[24] = {
#include "game/008CE7E0.inc"
};
u32 FireBlast_PartCurves_4[20] = {
#include "game/008CE840.inc"
};
u32 FireBlast_PartCurves_18[24] = {
#include "game/008CE890.inc"
};
u32 FireBlast_PartCurves_6[20] = {
#include "game/008CE8F0.inc"
};
u32 FireBlast_PartCurves_8[24] = {
#include "game/008CE940.inc"
};
u32 FireBlast_PartCurves_10[12] = {
#include "game/008CE9A0.inc"
};
u32 FireBlast_PartCurves_12[12] = {
#include "game/008CE9D0.inc"
};
u32 FireBlast_PartCurves_14[28] = {
#include "game/008CEA00.inc"
};
u32 FireBlast_PartCurves_16[12] = {
#include "game/008CEA70.inc"
};
u32 FireBlast_PartCurves_20[24] = {
#include "game/008CEAA0.inc"
};
u32 FireBlast_PartCurves_22[24] = {
#include "game/008CEB00.inc"
};
u32 FireBlast_PartCurves_24[16] = {
#include "game/008CEB60.inc"
};
u32 FireBlast_PartCurves_1[8] = {
#include "game/008CEBA0.inc"
};
u32 FireBlast_PartCurves_3[8] = {
#include "game/008CEBC0.inc"
};
u32 FireBlast_PartCurves_5[8] = {
#include "game/008CEBE0.inc"
};
u32 FireBlast_PartCurves_19[8] = {
#include "game/008CEC00.inc"
};
u32 FireBlast_PartCurves_7[8] = {
#include "game/008CEC20.inc"
};
u32 FireBlast_PartCurves_9[8] = {
#include "game/008CEC40.inc"
};
u32 FireBlast_PartCurves_11[8] = {
#include "game/008CEC60.inc"
};
u32 FireBlast_PartCurves_13[8] = {
#include "game/008CEC80.inc"
};
u32 FireBlast_PartCurves_15[8] = {
#include "game/008CECA0.inc"
};
u32 FireBlast_PartCurves_17[8] = {
#include "game/008CECC0.inc"
};
u32 FireBlast_PartCurves_21[8] = {
#include "game/008CECE0.inc"
};
u32 FireBlast_PartCurves_23[8] = {
#include "game/008CED00.inc"
};
u32 FireBlast_PartCurves_25[12] = {
#include "game/008CED20.inc"
};
u32 FireBlast_PartCurves_26[12] = {
#include "game/008CED50.inc"
};
u32 FireBlast_PartCurves_27[10] = {
#include "game/008CED80.inc"
};
s16 FireBlast_MineGlowModels[4] = {
#include "game/008CEDA8.inc"
};
ColorRgba FireBlast_MineRankColors[8] = {
#include "game/008CEDB0.inc"
};
void *FireBlast_PartCurves[28] = {
    (void *)FireBlast_PartCurves_0, (void *)FireBlast_PartCurves_1, (void *)FireBlast_PartCurves_2, (void *)FireBlast_PartCurves_3,
    (void *)FireBlast_PartCurves_4, (void *)FireBlast_PartCurves_5, (void *)FireBlast_PartCurves_6, (void *)FireBlast_PartCurves_7,
    (void *)FireBlast_PartCurves_8, (void *)FireBlast_PartCurves_9, (void *)FireBlast_PartCurves_10, (void *)FireBlast_PartCurves_11,
    (void *)FireBlast_PartCurves_12, (void *)FireBlast_PartCurves_13, (void *)FireBlast_PartCurves_14, (void *)FireBlast_PartCurves_15,
    (void *)FireBlast_PartCurves_16, (void *)FireBlast_PartCurves_17, (void *)FireBlast_PartCurves_18, (void *)FireBlast_PartCurves_19,
    (void *)FireBlast_PartCurves_20, (void *)FireBlast_PartCurves_21, (void *)FireBlast_PartCurves_22, (void *)FireBlast_PartCurves_23,
    (void *)FireBlast_PartCurves_24, (void *)FireBlast_PartCurves_25, (void *)FireBlast_PartCurves_26, (void *)FireBlast_PartCurves_27,
};
s16 FireBlast_BlastCellFrames[16] = {
#include "game/008CEE40.inc"
};
s16 FireBlast_SmallBlastCellFrames[16] = {
#include "game/008CEE60.inc"
};
u32 FireBlast_FlameShapes[16] = {
#include "game/008CEE80.inc"
};
s16 FireBlast_PartCounts[16] = {
#include "game/008CEEC0.inc"
};
u32 ElementBurst_Curves_0[16] = {
#include "game/008CEEE0.inc"
};
u32 ElementBurst_Curves_2[20] = {
#include "game/008CEF20.inc"
};
u32 ElementBurst_Curves_4[12] = {
#include "game/008CEF70.inc"
};
u32 ElementBurst_Curves_1[8] = {
#include "game/008CEFA0.inc"
};
u32 ElementBurst_Curves_3[8] = {
#include "game/008CEFC0.inc"
};
u32 ElementBurst_Curves_5[8] = {
#include "game/008CEFE0.inc"
};
s16 ElementBurst_DurLists_0[4] = {
#include "game/008CF000.inc"
};
s16 ElementBurst_FireDelays[4] = {
#include "game/008CF008.inc"
};
u32 ElementBurst_Curves_6[20] = {
#include "game/008CF010.inc"
};
u8 ElementBurst_PoisonColorKeys[48] = {
#include "game/008CF060.inc"
};
u32 ElementBurst_Curves_7[12] = {
#include "game/008CF090.inc"
};
u32 ElementBurst_Curves_8[12] = {
#include "game/008CF0C0.inc"
};
u32 ElementBurst_Curves_9[16] = {
#include "game/008CF0F0.inc"
};
u8 ElementBurst_WaterRingColorKeys[32] = {
#include "game/008CF130.inc"
};
s16 ElementBurst_DurLists_2[8] = {
#include "game/008CF150.inc"
};
u32 ElementBurst_Curves_10[28] = {
#include "game/008CF160.inc"
};
u32 ElementBurst_Curves_12[12] = {
#include "game/008CF1D0.inc"
};
u32 ElementBurst_Curves_11[8] = {
#include "game/008CF200.inc"
};
u32 ElementBurst_Curves_13[6] = {
#include "game/008CF220.inc"
};
s16 ElementBurst_DurLists_3[4] = {
#include "game/008CF238.inc"
};
u32 ElementBurst_Curves_14[20] = {
#include "game/008CF240.inc"
};
u32 ElementBurst_Curves_16[24] = {
#include "game/008CF290.inc"
};
u32 ElementBurst_Curves_18[12] = {
#include "game/008CF2F0.inc"
};
u32 ElementBurst_Curves_20[12] = {
#include "game/008CF320.inc"
};
u32 ElementBurst_Curves_24[12] = {
#include "game/008CF350.inc"
};
u32 ElementBurst_Curves_26[12] = {
#include "game/008CF380.inc"
};
u32 ElementBurst_Curves_15[8] = {
#include "game/008CF3B0.inc"
};
u32 ElementBurst_Curves_17[8] = {
#include "game/008CF3D0.inc"
};
u32 ElementBurst_Curves_19[8] = {
#include "game/008CF3F0.inc"
};
u32 ElementBurst_Curves_21[8] = {
#include "game/008CF410.inc"
};
u32 ElementBurst_Curves_25[8] = {
#include "game/008CF430.inc"
};
u32 ElementBurst_Curves_27[8] = {
#include "game/008CF450.inc"
};
s16 ElementBurst_RingDelays[8] = {
#include "game/008CF470.inc"
};
s16 ElementBurst_DurLists_4[8] = {
#include "game/008CF480.inc"
};
u8 ElementBurst_StatusColorKeys[40] = {
#include "game/008CF490.inc"
};
s16 ElementBurst_DurLists_9[4] = {
#include "game/008CF4B8.inc"
};
void *ElementBurst_Curves[28] = {
    (void *)ElementBurst_Curves_0, (void *)ElementBurst_Curves_1, (void *)ElementBurst_Curves_2, (void *)ElementBurst_Curves_3,
    (void *)ElementBurst_Curves_4, (void *)ElementBurst_Curves_5, (void *)ElementBurst_Curves_6, (void *)ElementBurst_Curves_7,
    (void *)ElementBurst_Curves_8, (void *)ElementBurst_Curves_9, (void *)ElementBurst_Curves_10, (void *)ElementBurst_Curves_11,
    (void *)ElementBurst_Curves_12, (void *)ElementBurst_Curves_13, (void *)ElementBurst_Curves_14, (void *)ElementBurst_Curves_15,
    (void *)ElementBurst_Curves_16, (void *)ElementBurst_Curves_17, (void *)ElementBurst_Curves_18, (void *)ElementBurst_Curves_19,
    (void *)ElementBurst_Curves_20, (void *)ElementBurst_Curves_21, (void *)ElementBurst_Curves_20, (void *)ElementBurst_Curves_21,
    (void *)ElementBurst_Curves_24, (void *)ElementBurst_Curves_25, (void *)ElementBurst_Curves_26, (void *)ElementBurst_Curves_27,
};
s16 ElementBurst_PartCounts[16] = {
#include "game/008CF530.inc"
};
s16 ElementBurst_Limits[16] = {
#include "game/008CF550.inc"
};
s16 ElementBurst_Steps[16] = {
#include "game/008CF570.inc"
};
s16 ElementBurst_CurveIdx[16] = {
#include "game/008CF590.inc"
};
s16 ElementBurst_Durations[16] = {
#include "game/008CF5B0.inc"
};
s16 *ElementBurst_DurLists[12] = {
    ElementBurst_DurLists_0, NULL, ElementBurst_DurLists_2, ElementBurst_DurLists_3,
    ElementBurst_DurLists_4, ElementBurst_DurLists_4, NULL, ElementBurst_DurLists_3,
    ElementBurst_DurLists_3, ElementBurst_DurLists_9, NULL, NULL,
};
u32 HitSplash_Curves_0[24] = {
#include "game/008CF600.inc"
};
u32 HitSplash_Curves_1[24] = {
#include "game/008CF660.inc"
};
u32 HitSplash_Curves_2[16] = {
#include "game/008CF6C0.inc"
};
u32 HitSplash_Curves_3[20] = {
#include "game/008CF700.inc"
};
u32 HitSplash_Curves_5[20] = {
#include "game/008CF750.inc"
};
u32 HitSplash_Curves_7[24] = {
#include "game/008CF7A0.inc"
};
u32 HitSplash_Curves_9[24] = {
#include "game/008CF800.inc"
};
u32 HitSplash_Curves_11[24] = {
#include "game/008CF860.inc"
};
u32 HitSplash_Curves_12[24] = {
#include "game/008CF8C0.inc"
};
u32 HitSplash_Curves_4[12] = {
#include "game/008CF920.inc"
};
u32 HitSplash_Curves_6[8] = {
#include "game/008CF950.inc"
};
u32 HitSplash_Curves_8[8] = {
#include "game/008CF970.inc"
};
u32 HitSplash_Curves_10[8] = {
#include "game/008CF990.inc"
};
s16 HitSplash_CellFrames[16] = {
#include "game/008CF9B0.inc"
};
u32 HitSplash_Curves[14] = {
    (u32)HitSplash_Curves_0, (u32)HitSplash_Curves_1, (u32)HitSplash_Curves_2, (u32)HitSplash_Curves_3,
    (u32)HitSplash_Curves_4, (u32)HitSplash_Curves_5, (u32)HitSplash_Curves_6, (u32)HitSplash_Curves_7,
    (u32)HitSplash_Curves_8, (u32)HitSplash_Curves_9, (u32)HitSplash_Curves_10, (u32)HitSplash_Curves_11,
    (u32)HitSplash_Curves_12, 0,
};
f32 HitSplash_TargetScales[3] = {
#include "game/008CFA08.inc"
};
HitSplashStep HitSplash_SpraySteps[8] = {
#include "game/008CFA20.inc"
};
HitSplashStep HitSplash_SprayStepsStrong[6] = {
#include "game/008CFA40.inc"
};
HitSplashStep HitSplash_SprayStepsAtOnce[2] = {
#include "game/008CFA58.inc"
};
s16 HitSplash_PartCounts[16] = {
#include "game/008CFA60.inc"
};
s16 HitSplash_Lives[16] = {
#include "game/008CFA80.inc"
};
s16 HitSplash_CurveIdx[16] = {
#include "game/008CFAA0.inc"
};
s16 HitSplash_Ends[16] = {
#include "game/008CFAC0.inc"
};
s16 HitSplash_Strides[16] = {
#include "game/008CFAE0.inc"
};
u32 AttackBurst_Curves_0[12] = {
#include "game/008CFB00.inc"
};
u32 AttackBurst_Curves_1[8] = {
#include "game/008CFB30.inc"
};
u32 AttackBurst_Curves_2[20] = {
#include "game/008CFB50.inc"
};
u32 AttackBurst_Curves_3[12] = {
#include "game/008CFBA0.inc"
};
u32 AttackBurst_Curves_4[8] = {
#include "game/008CFBD0.inc"
};
u32 AttackBurst_Curves_5[12] = {
#include "game/008CFBF0.inc"
};
u32 AttackBurst_Curves_7[12] = {
#include "game/008CFC20.inc"
};
u32 AttackBurst_Curves_6[8] = {
#include "game/008CFC50.inc"
};
u32 AttackBurst_Curves_8[8] = {
#include "game/008CFC70.inc"
};
u16 AttackBurst_Tbls_2[8] = {
#include "game/008CFC90.inc"
};
u32 AttackBurst_Curves_9[20] = {
#include "game/008CFCA0.inc"
};
u32 AttackBurst_Curves_10[16] = {
#include "game/008CFCF0.inc"
};
u8 AttackBurst_Curve3A[32] = {
#include "game/008CFD30.inc"
};
u8 AttackBurst_ColorKeys[48] = {
#include "game/008CFD50.inc"
};
u8 AttackBurst_Curve3B[32] = {
#include "game/008CFD80.inc"
};
u32 AttackBurst_Curves_11[24] = {
#include "game/008CFDA0.inc"
};
u32 AttackBurst_Curves_12[8] = {
#include "game/008CFE00.inc"
};
u32 AttackBurst_Curves_13[12] = {
#include "game/008CFE20.inc"
};
u8 AttackBurst_Curve10A[32] = {
#include "game/008CFE50.inc"
};
u32 AttackBurst_Curves_14[12] = {
#include "game/008CFE70.inc"
};
u32 AttackBurst_Curves_15[24] = {
#include "game/008CFEA0.inc"
};
u32 AttackBurst_Curves_16[24] = {
#include "game/008CFF00.inc"
};
u32 AttackBurst_Curves_18[24] = {
#include "game/008CFF60.inc"
};
u32 AttackBurst_Curves_17[8] = {
#include "game/008CFFC0.inc"
};
u32 AttackBurst_Curves_19[12] = {
#include "game/008CFFE0.inc"
};
u32 AttackBurst_Curves_20[16] = {
#include "game/008D0010.inc"
};
u32 AttackBurst_Curves_21[8] = {
#include "game/008D0050.inc"
};
u32 AttackBurst_Curves_22[16] = {
#include "game/008D0070.inc"
};
u32 AttackBurst_Curves_25[16] = {
#include "game/008D00B0.inc"
};
u32 AttackBurst_Curves_23[28] = {
#include "game/008D00F0.inc"
};
u32 AttackBurst_Curves_26[24] = {
#include "game/008D0160.inc"
};
u32 AttackBurst_Curves_24[8] = {
#include "game/008D01C0.inc"
};
u32 AttackBurst_Curves_27[8] = {
#include "game/008D01E0.inc"
};
u16 AttackBurst_Tbls_11[4] = {
#include "game/008D0200.inc"
};
s16 AttackBurst_DelaysA[4] = {
#include "game/008D0208.inc"
};
u32 AttackBurst_Curves_28[16] = {
#include "game/008D0210.inc"
};
u32 AttackBurst_Curves_29[12] = {
#include "game/008D0250.inc"
};
u32 AttackBurst_Curves_30[28] = {
#include "game/008D0280.inc"
};
u32 AttackBurst_Curves_31[12] = {
#include "game/008D02F0.inc"
};
u32 AttackBurst_Curves_32[12] = {
#include "game/008D0320.inc"
};
u32 AttackBurst_Curves_33[12] = {
#include "game/008D0350.inc"
};
u32 AttackBurst_Curves_34[8] = {
#include "game/008D0380.inc"
};
u32 AttackBurst_Curves_35[20] = {
#include "game/008D03A0.inc"
};
u32 AttackBurst_Curves_37[32] = {
#include "game/008D03F0.inc"
};
u32 AttackBurst_Curves_36[12] = {
#include "game/008D0470.inc"
};
u32 AttackBurst_Curves_38[12] = {
#include "game/008D04A0.inc"
};
u8 AttackBurst_CurveFA[32] = {
#include "game/008D04D0.inc"
};
u8 AttackBurst_CurveFB[48] = {
#include "game/008D04F0.inc"
};
u8 AttackBurst_CurveFC[32] = {
#include "game/008D0520.inc"
};
u16 AttackBurst_Tbls_15[4] = {
#include "game/008D0540.inc"
};
s16 AttackBurst_DelaysB[4] = {
#include "game/008D0548.inc"
};
u8 AttackBurst_Curve10B[32] = {
#include "game/008D0550.inc"
};
u32 AttackBurst_Curves_43[16] = {
#include "game/008D0570.inc"
};
u32 AttackBurst_Curves_44[8] = {
#include "game/008D05B0.inc"
};
u32 AttackBurst_Curves_45[12] = {
#include "game/008D05D0.inc"
};
u8 AttackBurst_Curve12A[32] = {
#include "game/008D0600.inc"
};
u8 AttackBurst_Curve12B[32] = {
#include "game/008D0620.inc"
};
void *AttackBurst_Curves[48] = {
    (void *)AttackBurst_Curves_0, (void *)AttackBurst_Curves_1, (void *)AttackBurst_Curves_2, (void *)AttackBurst_Curves_3,
    (void *)AttackBurst_Curves_4, (void *)AttackBurst_Curves_5, (void *)AttackBurst_Curves_6, (void *)AttackBurst_Curves_7,
    (void *)AttackBurst_Curves_8, (void *)AttackBurst_Curves_9, (void *)AttackBurst_Curves_10, (void *)AttackBurst_Curves_11,
    (void *)AttackBurst_Curves_12, (void *)AttackBurst_Curves_13, (void *)AttackBurst_Curves_14, (void *)AttackBurst_Curves_15,
    (void *)AttackBurst_Curves_16, (void *)AttackBurst_Curves_17, (void *)AttackBurst_Curves_18, (void *)AttackBurst_Curves_19,
    (void *)AttackBurst_Curves_20, (void *)AttackBurst_Curves_21, (void *)AttackBurst_Curves_22, (void *)AttackBurst_Curves_23,
    (void *)AttackBurst_Curves_24, (void *)AttackBurst_Curves_25, (void *)AttackBurst_Curves_26, (void *)AttackBurst_Curves_27,
    (void *)AttackBurst_Curves_28, (void *)AttackBurst_Curves_29, (void *)AttackBurst_Curves_30, (void *)AttackBurst_Curves_31,
    (void *)AttackBurst_Curves_32, (void *)AttackBurst_Curves_33, (void *)AttackBurst_Curves_34, (void *)AttackBurst_Curves_35,
    (void *)AttackBurst_Curves_36, (void *)AttackBurst_Curves_37, (void *)AttackBurst_Curves_38, (void *)AttackBurst_Curves_37,
    (void *)AttackBurst_Curves_38, (void *)AttackBurst_Curves_37, (void *)AttackBurst_Curves_38, (void *)AttackBurst_Curves_43,
    (void *)AttackBurst_Curves_44, (void *)AttackBurst_Curves_45, NULL, NULL,
};
s16 AttackBurst_PartCounts[24] = {
#include "game/008D0700.inc"
};
u16 AttackBurst_Durations[24] = {
#include "game/008D0730.inc"
};
s16 AttackBurst_CurveIdx[24] = {
#include "game/008D0760.inc"
};
s16 AttackBurst_Steps[24] = {
#include "game/008D0790.inc"
};
u16 *AttackBurst_Tbls[24] = {
    NULL, NULL, AttackBurst_Tbls_2, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, AttackBurst_Tbls_11,
    NULL, NULL, NULL, AttackBurst_Tbls_15,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
