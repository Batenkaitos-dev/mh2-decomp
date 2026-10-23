/*
 * The helpers the stage objects and their effects share (sub_main):
 * keyframe interpolation (Key_Lerp*, Color_EvalKeys), building a matrix
 * from a scale, rotations and a translation, drawing a material set faded
 * or coloured (Model_DrawFade, Effect_DrawModel*), the effects' colours,
 * scale and texture ids, random numbers in a range, and the few queries
 * objects make while they are set down (Scene_CanPlaceKind3,
 * Lure_PlayerHasMonsterBait, Player_DualSwordIndex).
 */
#include "common.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "render_state.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "rand.h"
#include "text.h"
#include "vec_math.h"

void Draw_SetBlendFlags(u16 flags);
extern u8 gAreaKindColors[];
extern u8 D_0041CF01[];
extern u8 D_0041CF02[];
extern u8 D_0041CF03[];
extern s16 gAreaKinds[];
extern s32 gStageObjFreeCount;          /* its number of free objects */
#define STAGEOBJ_COUNT 0x80             /* the pool's objects */
#define STAGEOBJ_COUNT 0x80             /* the pool's objects */
extern StageObj gStageObjs[STAGEOBJ_COUNT];
/* end of prototypes */

/*
 * Keys {time, x, y, z}, ended by a time of -1.0f: the vector at `time`, by
 * linear interpolation between the two keys around it; a key's own time gives
 * its value, and a time past the last key the last key's value.
 */
void Key_LerpVec3(f32 *keys, f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;

    for (;; keys += 4) {
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            out[1] = keys[2];
            out[2] = keys[3];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[4])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[5] - keys[1]);
            out[1] = keys[2] + frac * (keys[6] - keys[2]);
            out[2] = keys[3] + frac * (keys[7] - keys[3]);
            return;
        }
    }
    out[0] = keys[-3];
    out[1] = keys[-2];
    out[2] = keys[-1];
}

/*
 * Keys {time, x, y}, ended by a time of -1.0f: x and y at `time`, as
 * Key_LerpVec3 does, with out[2] set to 1. Past the last key the game writes
 * the key's y to out[2], not out[1]. The volatile keeps the store of 1.0f
 * inside the loop, where the game has it.
 */
void Key_LerpVec2(f32 *keys, volatile f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;
    for (;; keys += 3) {
        out[2] = 1.0f;
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            out[1] = keys[2];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[3])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[4] - keys[1]);
            out[1] = keys[2] + frac * (keys[5] - keys[2]);
            return;
        }
    }
    out[0] = keys[-2];
    out[2] = keys[-1];
}

/* Keys {time, value}, ended by a time of -1.0f: the value at `time`, as
 * Key_LerpVec3 does. */
void Key_LerpFloat(f32 *keys, f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;

    for (;; keys += 2) {
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[2])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[3] - keys[1]);
            return;
        }
    }
    out[0] = keys[-1];
}

/*
 * Colour keys {time, 0xAARRGGBB}, ended by a time of -1: the colour at
 * `time`, each channel interpolated between the two keys around it. color0
 * first holds time - time0: computed into a variable of its own, the
 * difference is scheduled before time1 - time0, as in the game.
 */
void Color_EvalKeys(s32 *keys, s32 time, u32 *out)
{
    f32 frac;
    s32 keyTime;
    s32 nextTime;
    u32 colorFrom;
    u32 colorTo;
    u8 redFrom;
    u8 alphaFrom;
    u8 greenFrom;
    u8 blueFrom;
    u8 alpha;
    u8 red;
    u8 green;
    u8 blue;

    for (;; keys += 2) {
        keyTime = keys[0];
        if (keyTime == -1) {
            break;
        }
        if (time == keyTime) {
            *out = keys[1];
            return;
        }
        nextTime = keys[2];
        if (time > keyTime && time < nextTime) {
            colorFrom = time - keyTime;
            colorTo = nextTime - keyTime;
            frac = (f32)(s32)colorFrom / (f32)(s32)colorTo;
            colorFrom = keys[1];
            colorTo = keys[3];
            redFrom = colorFrom >> 16;
            greenFrom = colorFrom >> 8;
            blueFrom = colorFrom;
            alphaFrom = colorFrom >> 24;
            alpha = alphaFrom + (u8)(frac * ((f32)(u8)(colorTo >> 24) - (f32)alphaFrom));
            red = redFrom + (u8)(frac * ((f32)(u8)(colorTo >> 16) - (f32)redFrom));
            green = greenFrom + (u8)(frac * ((f32)(u8)(colorTo >> 8) - (f32)greenFrom));
            blue = blueFrom + (u8)(frac * ((f32)(u8)colorTo - (f32)blueFrom));
            *out = (alpha << 24) | (red << 16) | (green << 8) | blue;
            return;
        }
    }
    *out = keys[-1];
}

/*
 * Builds mtx from a scale, rotations and a translation. rotOrder says which
 * rotations, in order: 0 none, 1 x, 2 y, 3 z, 4 x then y, 5 z then y, 6 the
 * three by Mtx_RotateXYZ, 7 by Mtx_RotateZXY.
 */
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 rotOrder, f32 *mtx)
{
    f32 x = trans[0];
    f32 y = trans[1];
    f32 z = trans[2];

    Mtx_Scale(mtx, scale[0], scale[1], scale[2]);
    switch (rotOrder) {
    case 6:
        Mtx_RotateXYZ(mtx, rot[0], rot[1], rot[2]);
        break;
    case 7:
        Mtx_RotateZXY(mtx, rot[0], rot[1], rot[2]);
        break;
    case 4:
        Mtx_RotateX(mtx, rot[0]);
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 5:
        Mtx_RotateZ(mtx, rot[2]);
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 1:
        Mtx_RotateX(mtx, rot[0]);
        break;
    case 2:
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 3:
        Mtx_RotateZ(mtx, rot[2]);
        break;
    case 0:
        break;
    }
    Mtx_SetTrans(mtx, x, y, z);
}

/*
 * Draws a material set (ModelMatSet, include/model.h) with the matrix mtx and
 * the materials mats, faded to `alpha` (0 to 1): white, or black when bit 0 of
 * flags is set; the other bits of flags go to Draw_SetBlendFlags. Returns 0
 * when there is nothing to draw, else 1.
 */
s32 Model_DrawFade(void *set, f32 *mtx, u16 flags, void *mats, f32 alpha)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    if (flags & 1) {
        Gs_SetState(RS_COLOR2, (u8)(255.0f * alpha) << 24);
    } else {
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * alpha) << 24) | 0xFFFFFF);
    }
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Draw_SetBlendFlags(flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/* Model_DrawFade with a colour, 0xAARRGGBB (the callers build it so),
 * instead of a fade. */
s32 Effect_DrawModel(void *set, f32 *mtx, s32 color, u16 flags, void *mats)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    Gs_SetState(RS_COLOR2, color);
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Draw_SetBlendFlags(flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/* Draws a material set with the matrix mtx, RS_ALPHA_REF set to 128 where
 * the other two set 0, and the set's own colour and blending. */
s32 Effect_DrawModelAdd(void *set, f32 *mtx, void *mats)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(RS_ALPHA_REF, 128);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/*
 * Sets the blending of the next draw from a model's draw flags: bit 4 calls
 * Gfx_SetTexFilter(GFX_TEX_NEAREST); bits 1 to 3 pick the blend: 2 Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE),
 * 4 Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA), 8 Gfx_SetBlend(GFX_BLEND_ONE, GFX_BLEND_ONE) with Gfx_SetBlendOp(GFX_BLEND_OP_REV_SUB).
 */
void Draw_SetBlendFlags(u16 flags)
{
    if (flags & 0x10) {
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
    }
    switch (flags & 0xE) {
    case 2:
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
        break;
    case 4:
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        break;
    case 8:
        Gfx_SetBlend(GFX_BLEND_ONE, GFX_BLEND_ONE);
        Gfx_SetBlendOp(GFX_BLEND_OP_REV_SUB);
        break;
    }
}

/*
 * An area's effect colour: its kind's entry of gAreaKindColors (four bytes,
 * r, g, b, a). The four bytes are read through four names (the table, and
 * three more names for its bytes at +1 to +3): the game loads each with its
 * own address, which one array read would not give.
 */
void Effect_GetColor(s32 area, u8 *red, u8 *green, u8 *blue, u8 *alpha)
{
    s32 ofs;

    ofs = gAreaKinds[area & 0xFFFF] * 4;
    *red = *(gAreaKindColors + ofs);
    *green = *(D_0041CF01 + ofs);
    *blue = *(D_0041CF02 + ofs);
    *alpha = *(D_0041CF03 + ofs);
}

/* Whether the local player may place another object counted by
 * gScene.placed3: fewer than 3 are out (ITEM_ID_MEAT and the ITEM_ID_BAIT_*
 * items test it before they are set down). */
s32 Scene_CanPlaceKind3(void)
{
    return (s32) gScene.placed3 < 3;
}

/*
 * Whether pl has a lure of type 1 out: a stage object of STAGEOBJ_KIND_LURE (the
 * lure) and type 1 owned by pl. Seen: Plesioth bites only that type, the
 * fish never do.
 */
s32 Lure_PlayerHasMonsterBait(void *pl)
{
    StageObj *obj;
    s16 i;

    i = 0;
    obj = gStageObjs;
    do {
        if (obj->active != 0 && obj->kind == 22 && obj->type == 1 && obj->owner == pl) {
            return 1;
        }
        i++;
        obj++;
    } while (i < STAGEOBJ_COUNT);
    return 0;
}

extern WeaponKindRange gDualSwordRanges[];

/*
 * For a player holding WEAPON_DUAL_SWORDS: its place in the
 * list of dual swords that gDualSwordRanges gives as ranges of weapon kinds
 * (-1 ends it), counted from 0 across the ranges; -1 for another weapon.
 * Seen: Bulldrome's effect keeps it per player.
 */
s16 Player_DualSwordIndex(void *obj)
{
    Player *pl = obj;
    s16 i;
    s16 index;
    s32 kind;

    if (pl->weaponType == WEAPON_DUAL_SWORDS) {
        for (i = 0, index = 0; gDualSwordRanges[i].lo != -1; i++) {
            kind = pl->weaponKind;
            if (kind >= gDualSwordRanges[i].lo && kind <= gDualSwordRanges[i].hi) {
                return index + kind - gDualSwordRanges[i].lo;
            }
            index += gDualSwordRanges[i].hi - gDualSwordRanges[i].lo + 1;
        }
    }
    return -1;
}

/* Scales a vector by Effect_Scale. */
void Effect_ScaleVec(f32 *vec)
{
    Vec_Scale((f32 *) vec, (f32 *) vec, Effect_Scale());
}

/* The effects' global scale: always 1.0f. */
f32 Effect_Scale(void)
{
    return 1.0f;
}

/*
 * The row of gEffectColors a stage object of STAGEOBJ_KIND_HIT_SPLASH takes from its
 * ownerIndex: 2 for 19, 24 and 56; 6 for 8 and 34;
 * 7 for 48, 55, 66, 67 and 73; 0 for the rest.
 */
s32 Effect_OwnerColorRow(s32 ownerIndex)
{
    s32 ownerKind;

    ownerKind = ownerIndex & 0xFF;
    switch (ownerKind) {
    case 19:
    case 24:
    case 56:
        return 2;
    case 8:
    case 34:
        return 6;
    case 48:
    case 55:
    case 66:
    case 73:
    case 67:
        return 7;
    default:
        return 0;
    }
}

/* A random number from 0 to 1.023, in steps of 0.001. */
f32 Rand_Unit(void)
{
    return 0.001f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
}

/* A random number from -1.024 to 1.022, in steps of 0.002. */
f32 Rand_Signed(void)
{
    return 0.002f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 512);
}

extern s16 gEffectTexIds[];

/*
 * An effect texture's id as loaded: offline the id itself; online, where only
 * the textures of gEffectTexIds are loaded, its index in that list (-1 ends
 * it), or -1 when it is not there.
 */
s16 Effect_MapTexId(s16 id)
{
    s16 i;

    if (gScene.inLobby == 0) {
        return id;
    }
    for (i = 0; gEffectTexIds[i] != -1; i++) {
        if (id == gEffectTexIds[i]) {
            return i;
        }
    }
    return -1;
}

/* The file's initialised data, in the game's order */
u8 gAreaKindColors[80] = {
#include "sub_main/0041CF00.inc"
};
WeaponKindRange gDualSwordRanges[12] = {
#include "sub_main/0041CF50.inc"
};
s16 gEffectTexIds[44] = {
#include "sub_main/0041CF80.inc"
};
