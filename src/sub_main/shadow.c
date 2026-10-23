/*
 * The shadows drawn under the players and the monsters
 * (STAGEOBJ_KIND_SHADOW, enum ShadowType in shadow.h): Shadow_Spawn makes
 * one for its owner, Shadow_Update runs it by state (set up, run, end,
 * free), and Shadow_Draw draws it by type: one of the common models, a
 * player's two part shadows, five parts under five bones, the owner's bones
 * laid on the ground (Bone_SnapToGround), or the monster's own model.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "sprite.h"
#include "area.h"
#include "render_state.h"
#include "lobby_state.h"
#include "shadow.h"
#include "stageobj.h"
#include "card.h"
#include "text.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "vec_math.h"
#include "em_blangonga.h"
#include "em_chameleos.h"

/* The game compiled this code at optimization level 4: at the default
 * level Shadow_Draw keeps a value in another register. */
#pragma optimization_level 4
/* The buffer of a five-part shadow (SHADOW_PARTS, StageObj.buf): one shadow part under each of
 * five bones of the owner (gShadowPartBones), a draw slot each. */
typedef struct EffectParts {
    f32 pos[5][3];              /* 0x00: the parts' positions */
    DrawSlot *draw[5];          /* 0x3C */
    s16 slot[5];                /* 0x50 */
} EffectParts;
/* end of prototypes */

/* The update of a shadow (STAGEOBJ_KIND_SHADOW, Shadow_Spawn), by state: set up, run, end,
 * free. */
void Shadow_Update(StageObj *shadow)
{
    switch (shadow->state) {
    case 0:
        Shadow_Setup(shadow);
        break;
    case 1:
        Shadow_Run(shadow);
        break;
    case 2:
        Shadow_End(shadow);
        break;
    case 3:
        Shadow_Delete(shadow);
        break;
    }
}

/* Sets up a shadow: its draw slots (SHADOW_PARTS: five, one per part; the others one, from the second
 * pool when keep is set), drawn by Shadow_Draw; with no slot free it is freed. Then runs its
 * first frame. */
void Shadow_Setup(StageObj *shadow)
{
    EffectParts *parts;
    s16 i;

    shadow->state++;
    shadow->var0 = 0;
    shadow->visible = 1;
    shadow->draw = NULL;
    switch (shadow->type) {
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        for (i = 0; i < 5; i++) {
            parts->slot[i] = DrawSlot_Alloc();
            if (parts->slot[i] != -1) {
                parts->draw[i] = DrawSlot_Get(parts->slot[i]);
                parts->draw[i]->owner = shadow;
                parts->draw[i]->part = i;
                parts->draw[i]->draw = Shadow_Draw;
            } else {
                parts->draw[i] = NULL;
            }
            shadow->node = NULL;
        }
        break;
    default:
        if (shadow->keep) {
            shadow->slot = DrawSlotB_Alloc();
        } else {
            shadow->slot = DrawSlot_Alloc();
        }
        if (shadow->slot != -1) {
            if (shadow->keep) {
                shadow->node = DrawSlotB_Get(shadow->slot);
            } else {
                shadow->node = DrawSlot_Get(shadow->slot);
            }
            shadow->node->owner = shadow;
            shadow->node->draw = Shadow_Draw;
            break;
        }
        Shadow_Delete(shadow);
        return;
    }
    Shadow_Run(shadow);
}

extern f32 gCamMatrix[];
extern s16 gShadowPartBones[];
extern f32 gShadowPartScales[];
extern f32 gShadowSizesLobby[][2];
extern f32 gShadowSizes[][2];
/* Shadow_Update's running state: follows the owner (SHADOW_PLAYER: the position of its
 * part 10 on the floor, and in the town only once its model is loaded; SHADOW_PARTS: five bones, each
 * part put 5 above the floor and drawn when its bone is 8 or more above it; the others: bone 2
 * on the floor), pushes the draw slot towards the camera for a monster (by -10 times the size
 * table's first value plus twice the kind, as the game computes it), and puts it in the ordering
 * table. The end state follows once the owner is gone. */
void Shadow_Run(StageObj *shadow)
{
    f32 offset[3];
    EffectParts *parts;
    f32 *sizes;
    s16 i;
    Player *pl;
    f32 groundY;

    pl = shadow->owner;
    switch (shadow->type) {
    case SHADOW_PLAYER:
        if (gScene.inLobby != 0) {
            if (pl->active == 0 || (s8)gLobby->pcLoaded[pl->index] == 0) { /* read as a signed byte, as the game does */
                return;
            }
        } else if (pl->active == 0) {
            return;
        }
        shadow->pos[0] = pl->partRecs[10]->m1[12];
        shadow->pos[1] = pl->floorY;
        shadow->pos[2] = pl->partRecs[10]->m1[14];
        goto draw; /* the player's shadow shares the default case's draw: the game's code jumps into it */
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        if (pl->active == 0) {
            shadow->state++;
            return;
        }
        if (pl->visible == 0 || (u8)Obj_InCurrentArea(pl) == 0) {
            return;
        }
        for (i = 0; i < 5; i++) {
            if (parts->draw[i] != NULL) {
                Mtx_GetTrans(parts->pos[i], Em_GetBone(pl, gShadowPartBones[i])->world);
                groundY = AreaGrid2_GetFloorY(parts->pos[i]);
                if (!(parts->pos[i][1] < 8.0f + groundY)) {
                    parts->pos[i][1] = 5.0f + groundY;
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = pl->scale[0] * (-10.0f * gShadowPartScales[i]);
                    Vec_RotateSelf(offset, gCamMatrix);
                    parts->draw[i]->pos[0] = offset[0] + parts->pos[i][0];
                    parts->draw[i]->pos[1] = offset[1] + parts->pos[i][1];
                    parts->draw[i]->pos[2] = offset[2] + parts->pos[i][2];
                    OT_AddByDepth(gDrawOtSorted, parts->draw[i], 64, 0);
                }
            }
        }
        return;
    default:
        if (pl->active == 0) {
            shadow->state++;
            return;
        }
        Em_BonePos(pl, 2, shadow->pos);
        shadow->pos[1] = pl->floorY;
    draw:
        if (gScene.inLobby == 0 || pl->ownTask != 1 || pl->weaponType != 3) { /* the town's monster of kind 3 is drawn in any area */
            if ((u8)Obj_InCurrentArea(pl) == 0) {
                return;
            }
        }
        if (pl->visible == 0) {
            return;
        }
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 0.0f;
        if (pl->isEm != 0) {
            if (gScene.inLobby != 0) {
                sizes = gShadowSizesLobby[0];
            } else {
                sizes = gShadowSizes[0];
            }
            offset[2] = pl->scale[0] * (-10.0f * (*sizes + pl->weaponType * 2));
            Vec_RotateSelf(offset, gCamMatrix);
        }
        shadow->node->pos[0] = shadow->pos[0] + offset[0];
        shadow->node->pos[1] = shadow->pos[1] + offset[1];
        shadow->node->pos[2] = shadow->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, shadow->node, 64, 0);
        break;
    }
}

/* Frees an effect object's draw slots (SHADOW_PARTS: the five of its buffer). */
void Shadow_End(StageObj *shadow)
{
    EffectParts *parts;
    s16 i;

    shadow->state++;
    switch (shadow->type) {
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        for (i = 0; i < 5; i++) {
            if (parts->draw[i] != NULL) {
                DrawSlot_Free(parts->slot[i]);
            }
        }
        break;
    default:
        if (shadow->node != NULL) {
            if (shadow->keep) {
                DrawSlotB_Free(shadow->slot);
            } else {
                DrawSlot_Free(shadow->slot);
            }
        }
        break;
    }
}

void Shadow_Delete(StageObj *shadow)
{
    StageObj_Free(shadow);
}

/* Sets the materials of the set matSet (from mats) for the draw, each with its alpha = the owner's
 * alpha times scale (the shadow's brightness). */
void Shadow_SetMaterials(ObjHead *owner, ModelMat *mats, ModelMatSet *matSet, f32 scale)
{
    s32 i;

    for (i = 0; i < matSet->count; i++) {
        ModelMat *mat = &mats[matSet->mat[i]];

        mat->alpha = owner->alpha * scale;
        Gs_SetState(RS_MATERIAL(i), (s32)mat);
    }
}

/* Walks a bone tree (child, then sibling): each bone's world matrix with its height set to the
 * ground's (clamped to base +-30) plus 5 + 0.3 * n, times the bone's skin matrix, into its matrix
 * slot (RS_MTX_SLOT(index)). Lays a shadow's bones on the ground (SHADOW_GROUND_BONES). */
void Bone_SnapToGround(EmBone *bone, u8 n, f32 base)
{
    f32 pos[3];
    f32 out[4][4];
    f32 mtx[4][4];
    f32 *y;
    f32 groundY;

    Mtx_Copy(mtx[0], bone->world);
    y = &mtx[3][1];
    pos[0] = mtx[3][0];
    pos[1] = *y;
    pos[2] = mtx[3][2];
    groundY = AreaGrid2_GetFloorY(pos);
    if (groundY > base + 30.0f) {
        groundY = base + 30.0f;
    } else if (groundY < base - 30.0f) {
        groundY = base - 30.0f;
    }
    *y = groundY + (5.0f + 0.3f * n);
    Mtx_Mul(out[0], bone->skin, mtx[0]);
    Gs_SetState(RS_MTX_SLOT(bone->index), (s32)out);
    if (bone->child != NULL) {
        Bone_SnapToGround(bone->child, n, base);
    }
    if (bone->sibling != NULL) {
        Bone_SnapToGround(bone->sibling, n, base);
    }
}

struct EmBone;
extern s16 gShadowBonesEm07[];
extern s16 gShadowBonesEm37[];
extern s16 gShadowBonesEm3A[];
/* Draws a shadow (the slot's owner) under its owner, by type: 0 and 3 one model of the common
 * models (scaled by the monster's shadow size, stretched along four bones for some kinds, faded
 * for the Blangonga family and an invisible Chameleos; 3 laid on the ground bone by bone), 1 a
 * player's two part shadows turned away from the sky's far point (half bright at night in a
 * quest), 2 one of the five parts, 4 the monster's own model set (Kirin). Skipped while the owner's
 * hideShadow is set, for a local player aiming, and outside the current area. */
void Shadow_Draw(DrawSlot *slot)
{
    f32 mtx[4][4];
    f32 out[4][4];
    f32 scaleMtx[4][4];
    f32 bonePos[4][3];
    f32 pos[3];
    StageObj *shadow;
    Model *set;
    f32 *sizes;
    ModelMatSet *matSet;
    ModelMat *mats;
    s32 i;
    Player *pl;
    s16 *bones;
    s16 idx;
    f32 scaleX;
    f32 scaleZ;
    f32 bright;
    u16 ang;
    f32 groundY;
    f32 spanX;
    f32 spanZ;
    EffectParts *w;

    shadow = slot->owner;
    pl = shadow->owner;
    if (pl->hideShadow != 0) {
        return;
    }
    if (shadow->type == SHADOW_MODEL || shadow->type == SHADOW_GROUND_BONES) {
        set = gCommonModels[1];
    } else if (shadow->type == SHADOW_OWN_MODEL) {
        idx = EmSlot_FindKind(pl->weaponType);
        if (idx == -1) {
            return;
        }
        set = gScene.partModels[idx];
    } else {
        set = gCommonModels[0];
    }
    if (pl->isEm == 0) {
        if (Player_IsLocal(pl) == 1 && pl->aimLockTimer != 0) {
            return;
        }
    } else if (gScene.inLobby != 0) {
        sizes = gShadowSizesLobby[0];
    } else {
        sizes = gShadowSizes[0];
    }
    if (gScene.inLobby == 0 || pl->ownTask != 1 || pl->weaponType != 3) { /* the town's monster of kind 3 is drawn in any area */
        if (gScene.area != pl->area) {
            return;
        }
    }
    if (set == NULL || pl->visible == 0) {
        return;
    }
    if (set->active == 0) {
        return;
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    mats = set->mats;
    switch (shadow->type) {
    case SHADOW_MODEL:
    case SHADOW_GROUND_BONES:
        matSet = set->matSets;
        Gs_SetState(RS_COLOR2, -1);
        if (pl->isEm == 0) {
            Mtx_Scale(scaleMtx[0], 4.0f, 1.0f, 4.0f);
            bright = 1.0f;
        } else {
            scaleX = pl->scale[0] * sizes[pl->weaponType * 2];
            scaleZ = pl->scale[2] * sizes[pl->weaponType * 2 + 1];
            switch (pl->weaponType) {
            case EM_KIND_LAO_SHAN_LUNG:
            case EM_KIND_ASHEN_LAO_SHAN_LUNG:
            case EM_KIND_SHEN_GAOREN:
            case EM_KIND_YAMA_TSUKAMI:
            case EM_KIND_YAMA_TSUKAMI_B:
                switch (pl->weaponType) {
                case EM_KIND_LAO_SHAN_LUNG:
                case EM_KIND_ASHEN_LAO_SHAN_LUNG:
                    bones = gShadowBonesEm07;
                    break;
                case EM_KIND_SHEN_GAOREN:
                    bones = gShadowBonesEm37;
                    break;
                case EM_KIND_YAMA_TSUKAMI:
                case EM_KIND_YAMA_TSUKAMI_B:
                    bones = gShadowBonesEm3A;
                    break;
                }
                matSet = &set->matSets[1];
                for (i = 0; i < 4; i++) {
                    Em_GetBonePos(pl, bones[i], bonePos[i]);
                    Vec_Diff(bonePos[i], bonePos[i], shadow->pos);
                    Vec_RotateY(bonePos[i], 6.2831855f * -pl->rot[1] / 65536.0f);
                }
                spanX = Math_Absf(0.5f * (bonePos[0][0] + bonePos[2][0]) - 0.5f * (bonePos[1][0] + bonePos[3][0]));
                spanZ = Math_Absf(0.5f * (bonePos[0][2] + bonePos[1][2]) - 0.5f * (bonePos[2][2] + bonePos[3][2]));
                scaleX += 0.02f * spanX;
                scaleZ += 0.02f * spanZ;
                bright = 1.0f;
                break;
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
            case EM_KIND_RAJANG:
                bright = EmBlangonga_ShadowFade((Em *) pl);
                break;
            case EM_KIND_CHAMELEOS:
                if (EmChameleos_IsInvisible((Em *) pl) == 1) {
                    bright = 0.0f;
                } else {
                    bright = 1.0f;
                }
                break;
            default:
                bright = 1.0f;
                break;
            }
            Mtx_Scale(scaleMtx[0], scaleX, 1.0f, scaleZ);
        }
        Mtx_Translation(mtx[0], shadow->pos[0], 3.0f + shadow->pos[1], shadow->pos[2]);
        Mtx_RotateY(mtx[0], Math_Angle16ToRad(((Player *)shadow->owner)->rot[1]));
        Mtx_Concat3x3(mtx[0], scaleMtx[0]);
        Bone_UpdateWorld(set->bones, mtx[0]);
        if (shadow->type == SHADOW_MODEL) {
            Bone_StoreMatrices(set->bones);
        } else {
            Bone_SnapToGround(set->bones, shadow->ownerIndex, pl->floorY);
        }
        if (matSet != NULL && matSet->id != -1) {
            Shadow_SetMaterials((ObjHead *)pl, mats, matSet, bright);
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        break;
    case SHADOW_PLAYER:
        idx = Effect_MapTexId(51);
        if (idx != -1) {
            matSet = &set->matSets[idx];
            Mtx_Scale(scaleMtx[0], 1.0f, 1.0f, 1.0f);
            for (i = 0; i < 2; i++) {
                if (i == 0) {
                    Mtx_GetTrans(pos, pl->partRecs[5]->m1);
                } else {
                    Mtx_GetTrans(pos, pl->partRecs[8]->m1);
                }
                pos[1] -= 13.0f;
                groundY = AreaGrid2_GetFloorY(pos);
                if (!(groundY < pos[1])) {
                    groundY = pos[1];
                }
                Mtx_Identity(mtx[0]);
                ang = Vec_Heading(pl->pos, gAreaState.skyPos) + ANGLE_TURN / 4;
                Mtx_RotateY(mtx[0], 6.2831855f * ang / 65536.0f);
                Mtx_SetTrans(mtx[0], pos[0], 4.0f + groundY, pos[2]);
                Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
                Gs_SetState(RS_MTX_SLOT(0), (s32)out);
                if (gScene.inLobby == 0 && Area_IsDay(gScene.area) == 0) {
                    bright = 0.5f;
                } else {
                    bright = 1.0f;
                }
                Shadow_SetMaterials((ObjHead *)pl, mats, matSet, bright);
                Draw_ApplyStates(matSet->flags);
                Model_Draw(matSet->id, 0);
            }
        }
        break;
    case SHADOW_PARTS:
        w = (EffectParts *)shadow->buf;
        idx = Effect_MapTexId(50);
        if (idx != -1) {
            matSet = &set->matSets[idx];
            Mtx_Scale(scaleMtx[0], gShadowPartScales[slot->part], gShadowPartScales[slot->part], gShadowPartScales[slot->part]);
            Mtx_Translation(mtx[0], w->pos[slot->part][0], w->pos[slot->part][1], w->pos[slot->part][2]);
            Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)out);
            Shadow_SetMaterials((ObjHead *)pl, mats, matSet, 1.0f);
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        break;
    case SHADOW_OWN_MODEL:
        matSet = &set->matSets[3];
        Mtx_Scale(scaleMtx[0], sizes[pl->weaponType * 2], 1.0f, sizes[pl->weaponType * 2 + 1]);
        Mtx_Translation(mtx[0], shadow->pos[0], 3.0f + shadow->pos[1], shadow->pos[2]);
        Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
        Gs_SetState(RS_MTX_SLOT(0), (s32)out);
        Gs_SetState(RS_COLOR2, 0x80FFFFFF);
        Shadow_SetMaterials((ObjHead *)pl, mats, matSet, 1.0f);
        Draw_ApplyStates(matSet->flags);
        Model_Draw(matSet->id, 0);
        break;
    }
    Gs_SetState(RS_ZWRITE, 1);
    Draw_SetDefaultState();
}

/* Makes the shadow of a player or monster (STAGEOBJ_KIND_SHADOW, of the given ShadowType; SHADOW_PARTS takes a
 * buffer block for its five parts): SHADOW_PLAYER is kept (keep) and placed by its run; the others start
 * at the owner's bone 10 on the floor. */
void Shadow_Spawn(void *obj, s16 type)
{
    Player *pl = obj;
    StageObj *shadow;

    if (type == 2) {
        if ((shadow = StageObj_Alloc(1)) == NULL) {
            return;
        }
    } else {
        if ((shadow = StageObj_Alloc(0)) == NULL) {
            return;
        }
    }
    shadow->kind = STAGEOBJ_KIND_SHADOW;
    shadow->update = Shadow_Update;
    shadow->draw = NULL;
    shadow->owner = pl;
    shadow->type = type;
    if (shadow->type == SHADOW_PLAYER) {
        shadow->keep = 1;
        return;
    }
    shadow->ownerIndex = pl->index;
    Em_BonePos(pl, 10, shadow->pos);
    shadow->pos[1] = pl->floorY;
}

/* The file's initialised data, in the game's order */
s16 gShadowPartBones[8] = {
#include "sub_main/004656C0.inc"
};
f32 gShadowPartScales[6] = {
#include "sub_main/004656D0.inc"
};
s16 gShadowBonesEm07[4] = {
#include "sub_main/004656E8.inc"
};
s16 gShadowBonesEm37[4] = {
#include "sub_main/004656F0.inc"
};
s16 gShadowBonesEm3A[4] = {
#include "sub_main/004656F8.inc"
};
