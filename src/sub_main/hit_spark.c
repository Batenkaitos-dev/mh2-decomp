/*
 * The hit sparks (STAGEOBJ_KIND_HIT_SPARK): the small effect objects shown
 * where a hit lands, spawned by HitSpark_SpawnForPlayer and the other
 * HitSpark_Spawn* calls. HitSpark_Update runs each by state: set up (a type
 * per kind of spark, with its own angle, scale and colour), timed, ended and
 * freed; HitSpark_Draw draws it as a material set of the first common model.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "sprite.h"
#include "render_state.h"
#include "stageobj.h"
#include "text.h"
#include "hit_spark.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "area.h"
#include "vec_math.h"
/* end of prototypes */

/* The update of the hit sparks (STAGEOBJ_KIND_HIT_SPARK; HitSpark_SpawnForPlayer and the like), by state. */
void HitSpark_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        HitSpark_Init(fx);
        break;
    case 1:
        HitSpark_RunTimed(fx);
        break;
    case 2:
        HitSpark_End(fx);
        break;
    case 3:
        HitSpark_Delete(fx);
        break;
    }
}

/* HitSpark_Update's first state for types 0-12: shows the spark, randomizes it per type (angle,
 * scale, colour; type 8 starts at a bone of its owner) and gives it a draw slot drawn by
 * HitSpark_Draw (none free: freed). The factors are written as the quotients 0.4f / 1000.0f and
 * 0.3f / 30.0f: the game's constants are those quotients, one bit above 0.0004f and 0.01f. */
void HitSpark_Init(StageObj *fx)
{
    f32 mtx[16];
    f32 offset[3];

    fx->state++;
    fx->visible = 1;
    fx->draw = NULL;
    fx->timer = 0;
    switch (fx->type) {
    case 0:
        fx->timer = -4;
        break;
    case 4:
        fx->angle = Rand_Next(1);
        break;
    case 5:
        fx->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        break;
    case 7:
        fx->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        fx->angle = Rand_Next(1);
        break;
    case 6:
        fx->var1 = Rand_Next(1) % 3;
        break;
    case 8:
        offset[0] = (0.3f / 30.0f) * ((Rand_Next(1) & 0x3FF) - 512);
        offset[1] = 5.0f + (0.3f / 30.0f) * (Rand_Next(1) & 0x3FF);
        offset[2] = 0.0f;
        Mtx_Copy(mtx, (f32 *)Em_GetBone(fx->owner, fx->var0));
        Vec_RotateSelf(offset, mtx);
        Mtx_GetTrans(fx->pos, mtx);
        fx->pos[0] += offset[0];
        fx->pos[1] += offset[1];
        fx->pos[2] += offset[2];
        break;
    case 9:
    case 10:
    case 11:
        if (fx->var0 & 1) {
            fx->timer = -8;
        }
        fx->var1 = Rand_Next(1) >> 8;
        fx->var2 = Rand_Next(1) >> 8;
        fx->angle = Rand_Next(1);
        break;
    case 12:
        fx->scale = 2.0f;
        break;
    case 2:
        break;
    case 1:
    case 3:
    default:
        fx->var1 = Rand_Next(1) >> 8;
        fx->var2 += ((Rand_Next(1) & 0xFFF) - 2048) >> 8;
        fx->angle += (s16)((Rand_Next(1) & 0xFFF) - ANGLE_TURN / 32);
        break;
    }
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = HitSpark_Draw;
        return;
    }
    StageObj_Free(fx);
}

extern f32 gCamMatrix[];

/* HitSpark_Update's timed kinds (types 0-12): the timer runs in steps of 2
 * to a life per type (then the next state, hidden); some drift up, types 4,
 * 6 and 12 are drawn pushed towards the camera. */
void HitSpark_RunTimed(StageObj *fx)
{
    s16 life;
    f32 offset[3];

    fx->timer += 2;
    switch (fx->type) {
    case 4:
        life = 8;
        break;
    case 5:
        life = 24;
        break;
    case 6:
        life = 8;
        break;
    case 7:
        life = 24;
        break;
    case 8:
        life = 32;
        break;
    case 9:
    case 10:
    case 11:
        life = 16;
        break;
    case 12:
        life = 8;
        break;
    default:
        life = 30;
        break;
    }
    if (fx->timer <= 0) {
        return;
    }
    if (fx->timer > life) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    if (fx->node == NULL) {
        return;
    }
    switch (fx->type) {
    case 0:
    case 2:
        if (fx->timer >= 16) {
            fx->pos[1] -= 4.0f;
        }
        break;
    case 3:
        if (fx->timer >= 10) {
            fx->pos[1] -= 5.0f;
        }
        break;
    case 4:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = -20.0f;
        Vec_RotateSelf(offset, gCamMatrix);
        fx->node->pos[0] = fx->pos[0] + offset[0];
        fx->node->pos[1] = fx->pos[1] + offset[1];
        fx->node->pos[2] = fx->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, fx->node, 64, 0);
        return;
    case 6:
    case 12:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 50.0f;
        Vec_RotateSelf(offset, gCamMatrix);
        fx->node->pos[0] = fx->pos[0] + offset[0];
        fx->node->pos[1] = fx->pos[1] + offset[1];
        fx->node->pos[2] = fx->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, fx->node, 64, 0);
        return;
    case 8:
        fx->pos[1] += -0.4f * fx->timer;
        break;
    }
    fx->node->pos[0] = fx->pos[0];
    fx->node->pos[1] = fx->pos[1];
    fx->node->pos[2] = fx->pos[2];
    OT_AddByDepth(gDrawOt32, fx->node, 32, 0);
}

void HitSpark_End(StageObj *fx)
{
    fx->state++;
    DrawSlot_Free(fx->slot);
}

void HitSpark_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

extern u8 gEffectColors[][4];
extern f32 gHitSparkScaleKeys6[];
extern f32 gHitSparkKeys12[];
extern s32 gHitSparkColorKeys12[];
void Draw_SetBlendFlags(u16 flags);

/* Draws a hit spark (STAGEOBJ_KIND_HIT_SPARK): per type, a material set of the first common model
 * (gCommonModels[0]->matSets) with its scale, rotation, UV offset and colour (white, a row of
 * gEffectColors by the owner, or keyed for type 12); type 0 is drawn twice, the second turned
 * half a circle. */
void HitSpark_Draw(DrawSlot *slot)
{
    Model *set;
    StageObj *fx;
    ModelMat *mats;
    ModelMatSet *matSet;
    u16 flags;
    s16 colorRow;
    u8 r, g, b;
    u8 uvRow;
    s16 id;
    f32 uvDiv;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scaleVec[3];
    u32 color;

    set = gCommonModels[0];
    fx = slot->owner;
    flags = 0;
    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Mtx_Identity(uvMtx);
        switch (fx->type) {
        case 0:
        case 1:
            Gfx_SetTexFilter(GFX_TEX_NEAREST);
            flags |= 0x10;
            Gs_SetState(RS_ALPHA_REF, 128);
            scaleVec[0] = 1.0f;
            scaleVec[1] = 1.0f;
            scaleVec[2] = 1.0f;
            if (fx->type == 0) {
                r = 255;
                colorRow = -1;
                g = r;
                b = r;
            } else {
                Effect_ScaleVec(scaleVec);
                colorRow = Effect_OwnerColorRow(fx->ownerIndex);
            }
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            uvRow = fx->type;
            Mtx_RotateZ(mtx, 6.2831855f * (u16)((fx->var1 << 8) + fx->var2) / 65536.0f);
            Mtx_RotateY(mtx, 6.2831855f * (u16)fx->angle / 65536.0f);
            matSet = &set->matSets[fx->timer / 2];
            Mtx_SetTrans(uvMtx, 0.0f, 0.125f * uvRow, 0.0f);
            break;
        case 2:
            Gfx_SetTexFilter(GFX_TEX_NEAREST);
            flags |= 0x10;
            Gs_SetState(RS_ALPHA_REF, 128);
            Mtx_Scale(mtx, 0.1f, 0.1f, 0.7f);
            Mtx_RotateZXY(mtx, 6.2831855f * (fx->var1 << 8) / 65536.0f, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f);
            matSet = &set->matSets[fx->timer / 2];
            Mtx_SetTrans(uvMtx, 0.75f, 0.25f, 0.0f);
            r = 255;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 3:
            Gs_SetState(RS_ALPHA_REF, RS_ALPHA_REF_OPAQUE);
            matSet = set->matSets + fx->timer / 2 + 97;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZXY(mtx, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f, 6.2831855f * (fx->var1 << 8) / 65536.0f);
            if (fx->var0 == 0) {
                Mtx_SetTrans(uvMtx, 0.75f, 0.25f, 0.0f);
            } else {
                Mtx_SetTrans(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            colorRow = Effect_OwnerColorRow(fx->ownerIndex);
            break;
        case 4:
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = set->matSets + fx->timer / 2 + 27;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            if (fx->var1 == 1) {
                r = 255;
                colorRow = -1;
                g = r;
                b = r;
                flags |= 2;
                Mtx_SetTrans(uvMtx, 0.125f, 0.0f, 0.0f);
            } else {
                Effect_ScaleVec(scaleVec);
                colorRow = Effect_OwnerColorRow(fx->ownerIndex);
                Mtx_SetTrans(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZ(mtx, 6.2831855f * fx->angle / 65536.0f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 5:
        case 7:
            if (fx->type == 5) {
                uvDiv = 24.0f;
                id = 53;
            } else {
                uvDiv = 24.0f;
                id = 76;
            }
            id = Effect_MapTexId(id);
            if (id == -1) {
                return;
            }
            matSet = &set->matSets[id];
            Gs_SetState(RS_ALPHA_REF, 128);
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_RotateY(mtx, 6.2831855f * fx->angle / 65536.0f);
            Mtx_SetTrans(uvMtx, 0.0f, fx->timer * (0.125f / uvDiv), 0.0f);
            r = 255;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 6:
            flags |= 2;
            Key_LerpVec3(gHitSparkScaleKeys6, scaleVec, fx->timer / 2.0f);
            scaleVec[0] *= fx->scale;
            scaleVec[1] *= fx->scale;
            scaleVec[2] *= fx->scale;
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = set->matSets + fx->var1 + 32;
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_SetTrans(uvMtx, 0.25f, 0.0f, 0.0f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            r = 255;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 8:
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = &set->matSets[39];
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_Concat3x3(mtx, gCamMatrix);
            r = 255;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 9:
        case 10:
        case 11:
            Gs_SetState(RS_ALPHA_REF, 128);
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_RotateXYZ(mtx, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f, 6.2831855f * (fx->var1 << 8) / 65536.0f);
            matSet = set->matSets + fx->timer / 2 + 134;
            colorRow = -1;
            if (fx->type == 9) {
                r = 255;
                g = r;
                b = r;
            } else if (fx->type == 10) {
                r = 95;
                g = r;
                b = r;
            } else {
                r = 191;
                g = r;
                b = r;
            }
            break;
        case 12:
            Gs_SetState(RS_ALPHA_REF, 0);
            flags |= 2;
            matSet = set->matSets;
            Key_LerpVec3(gHitSparkKeys12, scaleVec, fx->timer / 2.0f);
            colorRow = -2;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            Color_EvalKeys(gHitSparkColorKeys12, fx->timer / 2.0f, &color);
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZ(mtx, 1.308997f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        }
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Gs_SetState(RS_UV_MTX, (s32)uvMtx);
        if (colorRow == -2) {
            Gs_SetState(RS_COLOR2, color);
        } else {
            if (colorRow >= 0) {
                r = gEffectColors[colorRow][0];
                g = gEffectColors[colorRow][1];
                b = gEffectColors[colorRow][2];
            }
            Gs_SetState(RS_COLOR2, (r << 16) | 0xFF000000 | (g << 8) | b);
        }
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (matSet != NULL && matSet->id != -1) {
            Gs_SetLightList(mats, matSet);
            Draw_ApplyStates(matSet->flags);
            Draw_SetBlendFlags(flags);
            Model_Draw(matSet->id, 0);
        }
        if (fx->type == 0) {
            Gs_SetState(RS_COLOR2, (r << 16) | 0xFF000000 | (g << 8) | b);
            Mtx_TurnZ(mtx, 3.1415927f);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            if (matSet != NULL && matSet->id != -1) {
                Gs_SetLightList(mats, matSet);
                Draw_ApplyStates(matSet->flags);
                Draw_SetBlendFlags(flags);
                Model_Draw(matSet->id, 0);
            }
        }
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Draw_SetDefaultState();
        if (fx->type == 6) {
            Gs_SetState(RS_ALPHA_REF, 0);
        } else {
            Gs_SetState(RS_ALPHA_REF, 128);
        }
    }
}

/* the calls cast to u8: this unit's code takes the value as u8 */
/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, belonging to the player and turned away from its heading. */
void HitSpark_SpawnForPlayer(Player *pl, u8 type, f32 *pos)
{
    StageObj *fx;

    if (pl != NULL && (u8)Obj_InCurrentArea(pl) != 0) {
        fx = StageObj_Alloc(0);
        if (fx != NULL) {
            fx->kind = STAGEOBJ_KIND_HIT_SPARK;
            fx->update = HitSpark_Update;
            fx->type = type;
            fx->owner = pl;
            fx->scale = 1.0f;
            fx->pos[0] = pos[0];
            fx->pos[1] = pos[1];
            fx->pos[2] = pos[2];
            fx->angle = pl->hitAngle + ANGLE_TURN / 2;
        }
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, with the high bytes of a and b and a random angle. */
void HitSpark_SpawnRandomAngle(u16 angle1, u16 angle2, u8 type, f32 *pos)
{
    StageObj *fx;

    if ((fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->scale = 1.0f;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var1 = angle1 >> 8;
        fx->var2 = angle2 >> 8;
        fx->angle = Rand_Next(1);
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale, belonging to o. */
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = owner;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->var2 = 0;
        fx->angle = ang;
        if (((ObjHead *)owner)->isEm != 0) {
            fx->ownerIndex = ((Em *)owner)->type;
        } else {
            fx->ownerIndex = 0;
        }
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale. */
void HitSpark_SpawnAtScaled(u16 angle2, u16 ang, s32 type, f32 *pos, f32 scale)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var2 = angle2 >> 8;
        fx->angle = ang;
    }
}

/* A hit spark (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with scale, sub in var0. var1 is 1 for the hit
 * effect kinds 9-16 (kind: hitFx & HITFX_KIND_MASK); for the others it is 0, and when the owner is
 * a monster its kind goes to ownerIndex (the colour row of Effect_OwnerColorRow). */
void HitSpark_Spawn(void *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->ownerIndex = 0;
        switch (kind) {
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            fx->var1 = 1;
            break;
        default:
            fx->var1 = 0;
            if (((ObjHead *)owner)->isEm != 0) {
                fx->ownerIndex = ((Em *)owner)->type;
            }
            break;
        }
        fx->var2 = 0;
        fx->angle = 0;
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) belonging to owner, with the given scale. */
void HitSpark_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0) {
        fx = StageObj_Alloc(0);
        if (fx != NULL) {
            fx->kind = STAGEOBJ_KIND_HIT_SPARK;
            fx->update = HitSpark_Update;
            fx->type = type;
            fx->owner = owner;
            fx->scale = scale;
            fx->var0 = sub;
            fx->var2 = 0;
            fx->angle = 0;
        }
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, turned to ang. */
void HitSpark_SpawnAt(f32 *pos, s32 type, u16 ang)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = 1.0f;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->angle = ang;
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale. */
void HitSpark_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->var2 = 0;
        fx->angle = ang;
        fx->ownerIndex = 0;
    }
}

/* The file's initialised data, in the game's order */
f32 gHitSparkScaleKeys6[20] = {
#include "sub_main/00465700.inc"
};
f32 gHitSparkKeys12[12] = {
#include "sub_main/00465750.inc"
};
s32 gHitSparkColorKeys12[8] = {
#include "sub_main/00465780.inc"
};
